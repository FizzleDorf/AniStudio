#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "ImageSystem.hpp"
#include "ImageUtils.hpp"
#include "SDCPPComponents.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPool.hpp"
#include "PngMetadataUtils.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ECS {

	// Forward declarations for Task classes
	class InferenceTask;
	class ConvertTask;
	class Img2ImgTask;
	class UpscalingTask;

	// static rng variables
	static STDDefaultRNG rng;
	static std::random_device rd;
	static bool initialized = false;

	class SDCPPSystem : public BaseSystem {
	public:
		enum class TaskType {
			Inference,
			Conversion,
			Img2Img,
			Upscaling
		};

		struct QueueItem {
			EntityID entityID = 0;
			bool processing = false;
			nlohmann::json metadata = nlohmann::json();
			std::shared_ptr<Utils::Task> task;
			TaskType taskType;
		};

		// Function to generate a random seed using STDDefaultRNG from sdcpp
		uint64_t generateRandomSeed() {
			// Seed the RNG with a random device if not already seeded
			if (!initialized) {
				rng.manual_seed(rd());
				initialized = true;
			}

			// Get random numbers
			std::vector<float> random_values = rng.randn(1);

			// Convert to a positive integer seed
			uint64_t seed = static_cast<uint64_t>(std::abs(random_values[0] * UINT32_MAX)) % INT32_MAX;
			return seed > 0 ? seed : 1; // Ensure seed is positive
		}

		// Constructor
		SDCPPSystem(EntityManager& entityMgr, size_t numThreads = 0)
			: BaseSystem(entityMgr),
			threadPool(numThreads > 0 ? numThreads : std::thread::hardware_concurrency() / 2) {
			sysName = "SDCPPSystem";
			AddComponentSignature<LatentComponent>();
			AddComponentSignature<OutputImageComponent>();
			AddComponentSignature<InputImageComponent>();

			activeTasks = 0;
		}

		// Destructor
		~SDCPPSystem() {

			std::unique_lock<std::mutex> lock(queueMutex);

			// Cancel all tasks
			for (auto& item : taskQueue) {
				if (item.task) {
					item.task->cancel();
				}
			}

			lock.unlock();

			if (sd_context) {
				free_sd_ctx(sd_context);
			}
			// Give tasks time to gracefully terminate
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		// Public methods
		void QueueTask(const EntityID entityID, const TaskType taskType) {
			// Validate entity exists first
			if (!mgr.GetEntitiesSignatures().count(entityID)) {
				std::cerr << "Error: Entity " << entityID << " does not exist!" << std::endl;
				return;
			}

			try {
				// try serializing entity before locking mutex
				QueueItem item;

				// Check if we need to generate a random seed
				if (taskType == TaskType::Inference || taskType == TaskType::Img2Img) {
					// Access the sampler component
					if (mgr.HasComponent<SamplerComponent>(entityID)) {
						auto& samplerComp = mgr.GetComponent<SamplerComponent>(entityID);

						// Generate random seed if needed
						if (samplerComp.seed < 0) {
							uint64_t newSeed = generateRandomSeed();
							samplerComp.seed = static_cast<int>(newSeed);

							std::cout << "Generated random seed: " << samplerComp.seed << std::endl;

							// Update metadata with new seed for logging
							if (item.metadata.contains("components") && item.metadata["components"].is_array()) {
								for (auto& comp : item.metadata["components"]) {
									if (comp.contains("Sampler")) {
										comp["Sampler"]["seed"] = samplerComp.seed;
										break;
									}
								}
							}
						}
					}
				}
				item.metadata = mgr.SerializeEntity(entityID);
				std::cout << "Successfully serialized entity " << entityID << std::endl;

				// lock and add to queue
				std::lock_guard<std::mutex> lock(queueMutex);

				item.entityID = entityID;
				item.processing = false;
				item.taskType = taskType;

				// Add to queue with normal copying instead of move to see if that helps
				taskQueue.push_back(item);

				std::cout << "Entity " << entityID << " queued for inference. New queue size: " << taskQueue.size() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in QueueTask: " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in QueueTask!" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			// Process queues - start any pending tasks
			ProcessQueues();

			// Update status of running tasks
			CheckTaskCompletion();
		}

		void RemoveFromQueue(const size_t index) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (index < taskQueue.size() && !taskQueue[index].processing) {
				mgr.DestroyEntity(taskQueue[index].entityID);
				taskQueue.erase(taskQueue.begin() + index);
			}
		}

		void MoveInQueue(const size_t fromIndex, const size_t toIndex) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size())
				return;
			if (taskQueue[fromIndex].processing)
				return;

			QueueItem item = taskQueue[fromIndex];
			taskQueue.erase(taskQueue.begin() + fromIndex);
			taskQueue.insert(taskQueue.begin() + toIndex, item);
		}

		std::vector<QueueItem> GetQueueSnapshot() {
			std::lock_guard<std::mutex> lock(queueMutex);
			return taskQueue;
		}

		void StopCurrentTask() {
			std::lock_guard<std::mutex> lock(queueMutex);
			taskQueue.remove(taskQueue.begin());
			/*for (auto& item : taskQueue) {
				if (item.processing && item.task) {
					item.task->cancel();
				}
			}*/
		}

		void ClearQueue() {
			if (taskQueue.empty()) {
				return;
			}
			for (size_t i = taskQueue.size() - 1; i > 0; --i) {
				RemoveFromQueue(i);
			}
		}

		void PauseWorker() {
			std::lock_guard<std::mutex> lock(queueMutex);
			pauseWorker = true;

			std::cout << "Worker paused. Tasks will continue running but no new tasks will be started." << std::endl;
		}

		void ResumeWorker() {
			std::lock_guard<std::mutex> lock(queueMutex);
			pauseWorker = false;

			std::cout << "Worker resumed. New tasks will now be processed." << std::endl;
		}

		// Thread pool stats
		size_t GetNumThreads() const { return threadPool.size(); }
		size_t GetQueuedTaskCount() const { return threadPool.getQueueSize(); }
		size_t GetActiveTaskCount() const { return activeTasks; }

	private:
		// Private member variables
		Utils::ThreadPool threadPool;
		std::atomic<size_t> activeTasks;
		std::vector<QueueItem> taskQueue;
		std::atomic<bool> pauseWorker{ false };
		std::mutex queueMutex;
		sd_ctx_t* sd_context = nullptr;

		// Friend classes for tasks
		friend class InferenceTask;
		friend class ConvertTask;
		friend class Img2ImgTask;
		friend class UpscalingTask;

		// Helper methods to create tasks
		std::shared_ptr<Utils::Task> CreateInferenceTask(const EntityID entityID, const nlohmann::json& metadata);
		std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID, const nlohmann::json& metadata);
		std::shared_ptr<Utils::Task> CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata);
		std::shared_ptr<Utils::Task> CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata);

		// Core methods that will be used by task classes
		bool RunInference(const EntityID entityID, const nlohmann::json& metadata);
		bool ConvertToGGUF(EntityID entityID, const nlohmann::json& metadata);
		bool RunImg2Img(EntityID entityID, const nlohmann::json& metadata);
		bool RunUpscaling(EntityID entityID, const nlohmann::json& metadata);

		// Queue processing methods
		void ProcessQueues() {
			std::lock_guard<std::mutex> lock(queueMutex);

			// If paused, don't process any new tasks
			if (pauseWorker) {
				return;
			}

			// Process task queue
			for (auto& item : taskQueue) {
				if (!item.processing && activeTasks < 1) {
					// Create a task based on the task type
					switch (item.taskType) {
					case TaskType::Inference:
						item.task = CreateInferenceTask(item.entityID, item.metadata);
						break;
					case TaskType::Conversion:
						item.task = CreateConversionTask(item.entityID, item.metadata);
						break;
					case TaskType::Img2Img:
						item.task = CreateImg2ImgTask(item.entityID, item.metadata);
						break;
					case TaskType::Upscaling:
						item.task = CreateUpscalingTask(item.entityID, item.metadata);
						break;
					default:
						continue;
					}

					item.processing = true;
					activeTasks++;

					// Add to thread pool
					threadPool.addTask(item.task);

					// Only start one task per update
					break;
				}
			}
		}

		void CheckTaskCompletion() {
			if (taskQueue.empty()) {
				return;
			}

			std::unique_lock<std::mutex> lock(queueMutex);

			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				if (it->processing && it->task && it->task->isDone()) {
					// Store the entity ID before erasing the item
					EntityID entityID = it->entityID;

					// Decrease active tasks count and remove from queue
					activeTasks--;
					it = taskQueue.erase(it);

					// Release the lock temporarily to avoid potential deadlocks
					lock.unlock();
					ProcessCompletedTask(entityID);
					lock.lock();
				}
				else {
					++it;
				}
			}
		}

		void ProcessCompletedTask(const EntityID entityID) {
			if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
				std::cerr << "Entity " << entityID << " missing OutputImageComponent" << std::endl;
				return;
			}

			try {
				if (!mgr.GetEntitiesSignatures().count(entityID)) {
					std::cerr << "Entity " << entityID << " no longer exists" << std::endl;
					return;
				}

				auto& outputComp = mgr.GetComponent<OutputImageComponent>(entityID);

				if (outputComp.filePath.empty()) {
					std::cerr << "ProcessCompletedTask: No filepath in OutputImageComponent for entity " << entityID << std::endl;
					return;
				}
				mgr.AddComponent<ImageComponent>(entityID);
				auto imageSystem = mgr.GetSystem<ImageSystem>();
				imageSystem->SetImage(entityID, outputComp.filePath);

				// Clean up entity - remove all components except ImageComponent
				std::vector<ComponentTypeID> componentTypes = mgr.GetEntityComponents(entityID);
				for (const auto& componentId : componentTypes) {
					std::string componentName = mgr.GetComponentNameById(componentId);
					if (componentName != "Image") {
						mgr.RemoveComponentById(entityID, componentId);
					}
				}

				std::cout << "Successfully processed completed task for entity " << entityID << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Error processing completed task: " << e.what() << std::endl;
			}
		}

		// SD context initialization
		sd_ctx_t* InitializeStableDiffusionContext(const nlohmann::json& metadata) {
			try {
				std::string modelPath = "", clipLPath = "", clipGPath = "", t5xxlPath = "";
				std::string diffusionModelPath = "", vaePath = "", taesdPath = "", controlnetPath = "";
				std::string loraPath = "", embedPath = "";
				bool vae_decode_only = false, isTiled = false, free_params_immediately = true;
				bool keep_vae_on_cpu = false;
				int n_threads = 4;
				sd_type_t type_method = SD_TYPE_F16;
				rng_type_t rng_type = STD_DEFAULT_RNG;
				schedule_t scheduler_method = DEFAULT;

				// Extract parameters from components array in metadata
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						// Model component
						if (comp.contains("Model")) {
							auto model = comp["Model"];
							if (model.contains("modelPath"))
								modelPath = model["modelPath"];
							else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
								modelPath = filePaths.checkpointDir + "/" + model["modelName"].get<std::string>();
						}

						// ClipL component
						if (comp.contains("ClipL")) {
							auto clipL = comp["ClipL"];
							if (clipL.contains("modelPath"))
								clipLPath = clipL["modelPath"];
							else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
								clipLPath = filePaths.encoderDir + "/" + clipL["modelName"].get<std::string>();
						}

						// ClipG component
						if (comp.contains("ClipG")) {
							auto clipG = comp["ClipG"];
							if (clipG.contains("modelPath"))
								clipGPath = clipG["modelPath"];
							else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
								clipGPath = filePaths.encoderDir + "/" + clipG["modelName"].get<std::string>();
						}

						// T5XXL component
						if (comp.contains("T5XXL")) {
							auto t5xxl = comp["T5XXL"];
							if (t5xxl.contains("modelPath"))
								t5xxlPath = t5xxl["modelPath"];
							else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
								t5xxlPath = filePaths.encoderDir + "/" + t5xxl["modelName"].get<std::string>();
						}

						// DiffusionModel component
						if (comp.contains("DiffusionModel")) {
							auto diffusion = comp["DiffusionModel"];
							if (diffusion.contains("modelPath"))
								diffusionModelPath = diffusion["modelPath"];
							else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
								diffusionModelPath = filePaths.unetDir + "/" + diffusion["modelName"].get<std::string>();
						}

						// Vae component
						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath"))
								vaePath = vae["modelPath"];
							else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty())
								vaePath = filePaths.vaeDir + "/" + vae["modelName"].get<std::string>();

							if (vae.contains("isTiled"))
								isTiled = vae["isTiled"];
							if (vae.contains("keep_vae_on_cpu"))
								keep_vae_on_cpu = vae["keep_vae_on_cpu"];
							if (vae.contains("vae_decode_only"))
								vae_decode_only = vae["vae_decode_only"];
						}

						// Taesd component
						if (comp.contains("Taesd")) {
							auto taesd = comp["Taesd"];
							if (taesd.contains("modelPath"))
								taesdPath = taesd["modelPath"];
							else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
								taesdPath = filePaths.vaeDir + "/" + taesd["modelName"].get<std::string>();
						}

						// Controlnet component
						if (comp.contains("Controlnet")) {
							auto controlnet = comp["Controlnet"];
							if (controlnet.contains("modelPath"))
								controlnetPath = controlnet["modelPath"];
							else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
								controlnetPath = filePaths.controlnetDir + "/" + controlnet["modelName"].get<std::string>();
						}

						// Lora component
						if (comp.contains("Lora")) {
							auto lora = comp["Lora"];
							if (lora.contains("modelPath"))
								loraPath = lora["modelPath"];
						}

						// Embedding component
						if (comp.contains("EmbeddingComponent")) {
							auto embed = comp["EmbeddingComponent"];
							if (embed.contains("modelPath"))
								embedPath = embed["modelPath"];
							else if (embed.contains("modelName") && !embed["modelName"].get<std::string>().empty())
								embedPath = filePaths.embedDir + "/" + embed["modelName"].get<std::string>();
						}

						// Sampler component
						if (comp.contains("Sampler")) {
							auto sampler = comp["Sampler"];
							if (sampler.contains("n_threads"))
								n_threads = sampler["n_threads"];
							if (sampler.contains("free_params_immediately"))
								free_params_immediately = sampler["free_params_immediately"];
							if (sampler.contains("current_type_method"))
								type_method = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
							if (sampler.contains("current_rng_type"))
								rng_type = static_cast<rng_type_t>(sampler["current_rng_type"].get<int>());
							if (sampler.contains("current_scheduler_method"))
								scheduler_method = static_cast<schedule_t>(sampler["current_scheduler_method"].get<int>());
						}
					}
				}

				// Initialize SD context with parsed metadata
				return new_sd_ctx(
					modelPath.c_str(),
					clipLPath.c_str(),
					clipGPath.c_str(),
					t5xxlPath.c_str(),
					diffusionModelPath.c_str(),
					vaePath.c_str(),
					taesdPath.c_str(),
					controlnetPath.c_str(),
					filePaths.loraDir.c_str(),
					embedPath.c_str(),
					"",  // placeholder_token_text
					vae_decode_only,
					isTiled,
					free_params_immediately,
					n_threads,
					type_method,
					rng_type,
					scheduler_method,
					true,  // shift_text_decoder
					false, // debug_clip_pos
					keep_vae_on_cpu,
					false  // debug_extract_shifts
				);
			}
			catch (const std::exception& e) {
				std::cerr << "Error initializing SD context: " << e.what() << std::endl;
				return nullptr;
			}
		}

		// Generate image based on metadata
		sd_image_t* GenerateImage(sd_ctx_t* context, const nlohmann::json& metadata) {
			std::string posPrompt = "", negPrompt = "";
			float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
			int latentWidth = 512, latentHeight = 512, steps = 20, seed = -1, batchSize = 1;
			sample_method_t sample_method = EULER;
			int* skipLayers = nullptr;
			size_t skipLayersCount = 0;
			float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;

			// Extract parameters from components array in metadata
			if (metadata.contains("components") && metadata["components"].is_array()) {
				for (const auto& comp : metadata["components"]) {
					// Prompt component
					if (comp.contains("Prompt")) {
						auto prompt = comp["Prompt"];
						if (prompt.contains("posPrompt"))
							posPrompt = prompt["posPrompt"];
						if (prompt.contains("negPrompt"))
							negPrompt = prompt["negPrompt"];
					}

					// ClipSkip component
					if (comp.contains("ClipSkip")) {
						auto clipSkipComp = comp["ClipSkip"];
						if (clipSkipComp.contains("clipSkip"))
							clipSkip = clipSkipComp["clipSkip"];
					}

					// Sampler component
					if (comp.contains("Sampler")) {
						auto sampler = comp["Sampler"];
						if (sampler.contains("cfg"))
							cfg = sampler["cfg"];
						if (sampler.contains("steps"))
							steps = sampler["steps"];
						if (sampler.contains("seed"))
							seed = sampler["seed"];
						if (sampler.contains("current_sample_method"))
							sample_method = static_cast<sample_method_t>(sampler["current_sample_method"].get<int>());
					}

					// Guidance component
					if (comp.contains("Guidance")) {
						auto guidanceComp = comp["Guidance"];
						if (guidanceComp.contains("guidance"))
							guidance = guidanceComp["guidance"];
						if (guidanceComp.contains("eta"))
							eta = guidanceComp["eta"];
					}

					// Latent component
					if (comp.contains("Latent")) {
						auto latent = comp["Latent"];
						if (latent.contains("latentWidth"))
							latentWidth = latent["latentWidth"];
						if (latent.contains("latentHeight"))
							latentHeight = latent["latentHeight"];
						if (latent.contains("batchSize"))
							batchSize = latent["batchSize"];
					}

					// Skip layers component
					if (comp.contains("LayerSkip")) {
						auto layerSkip = comp["LayerSkip"];
						if (layerSkip.contains("skip_layers"))
							skipLayers = reinterpret_cast<int*>(layerSkip["skip_layers"].get<intptr_t>());
						if (layerSkip.contains("skip_layers_count"))
							skipLayersCount = layerSkip["skip_layers_count"];
						if (layerSkip.contains("slg_scale"))
							slgScale = layerSkip["slg_scale"];
						if (layerSkip.contains("skip_layer_start"))
							skipLayerStart = layerSkip["skip_layer_start"];
						if (layerSkip.contains("skip_layer_end"))
							skipLayerEnd = layerSkip["skip_layer_end"];
					}
				}
			}

			// Call the Stable Diffusion txt2img function with extracted parameters
			return txt2img(
				context,
				posPrompt.c_str(),
				negPrompt.c_str(),
				clipSkip,
				cfg,
				guidance,
				eta,
				latentWidth,
				latentHeight,
				sample_method,
				steps,
				seed,
				batchSize,
				nullptr,  // control_image
				0.0f,     // control_strength
				0.0f,     // style_strength
				false,    // normalize_input
				"",       // input_id_images_path
				skipLayers,
				skipLayersCount,
				slgScale,
				skipLayerStart,
				skipLayerEnd
			);
		}

		// Save image with metadata
		void SaveImage(const unsigned char* data, int width, int height, int channels, EntityID entityID, const nlohmann::json& metadata) {
			try {
				// Make sure entity has OutputImageComponent
				if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
					// Add it if it doesn't exist yet
					mgr.AddComponent<OutputImageComponent>(entityID);
				}

				OutputImageComponent& outputComp = mgr.GetComponent<OutputImageComponent>(entityID);

				// Free any existing image data
				if (outputComp.imageData) {
					Utils::ImageUtils::FreeImageData(outputComp.imageData);
					outputComp.imageData = nullptr;
				}

				// Allocate and copy the image data
				size_t dataSize = width * height * channels;
				outputComp.imageData = (unsigned char*)malloc(dataSize);
				if (!outputComp.imageData) {
					std::cerr << "Failed to allocate memory for image data" << std::endl;
					return;
				}
				memcpy(outputComp.imageData, data, dataSize);

				// Update image component properties
				outputComp.width = width;
				outputComp.height = height;
				outputComp.channels = channels;

				// Create a path for the filename and ensure .png extension
				std::filesystem::path filename(outputComp.fileName);
				if (filename.extension() != ".png") {
					filename.replace_extension(".png");
					outputComp.fileName = filename.string();
				}

				// Use PngMetadata utility to get a unique filename
				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputComp.fileName,
					outputComp.filePath,
					".png"
				);

				// Update the component with the new path
				std::filesystem::path fullPath(uniqueFilePath);
				outputComp.fileName = fullPath.filename().string();
				outputComp.filePath = uniqueFilePath;

				// Save the image
				bool saved = Utils::ImageUtils::SaveImage(
					uniqueFilePath,
					width,
					height,
					channels,
					data
				);

				if (!saved) {
					std::cerr << "Failed to save image: " << uniqueFilePath << std::endl;
					return;
				}

				// Write metadata to PNG
				Utils::PngMetadata::WriteMetadataToPNG(uniqueFilePath, metadata, "AniStudio");

				std::cout << "Image saved successfully: \"" << uniqueFilePath << "\"" << std::endl;
			}
			catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Error creating directory: " << e.what() << '\n';
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in SaveImage: " << e.what() << '\n';
			}
		}
	}; // End of SDCPPSystem class

	// Task classes
	class InferenceTask : public Utils::Task {
	public:
		InferenceTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
			: system(system), entityID(entityID), metadata(metadata) {
		}

		void execute() override {
			if (!system || isCancelled()) return;

			// Run the inference
			try {
				// Mark the task as done when it completes (even if there's an error)
				bool success = system->RunInference(entityID, metadata);
				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in inference task: " << e.what() << std::endl;
				markDone();
			}
		}

	private:
		SDCPPSystem* system;
		EntityID entityID;
		nlohmann::json metadata;
	};

	class ConvertTask : public Utils::Task {
	public:
		ConvertTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
			: system(system), entityID(entityID), metadata(metadata) {
		}

		void execute() override {
			if (!system || isCancelled()) return;

			// Run the conversion
			try {
				bool success = system->ConvertToGGUF(entityID, metadata);
				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in conversion task: " << e.what() << std::endl;
				markDone();
			}
		}

	private:
		SDCPPSystem* system;
		EntityID entityID;
		nlohmann::json metadata;
	};

	class Img2ImgTask : public Utils::Task {
	public:
		Img2ImgTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
			: system(system), entityID(entityID), metadata(metadata) {
		}

		void execute() override {
			if (!system || isCancelled()) return;

			try {
				bool success = system->RunImg2Img(entityID, metadata);
				if(!success)
					throw std::runtime_error("Failed to generate image!");

				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in img2img task: " << e.what() << std::endl;
				markDone();
			}
		}

	private:
		SDCPPSystem* system;
		EntityID entityID;
		nlohmann::json metadata;
	};

	class UpscalingTask : public Utils::Task {
	public:
		UpscalingTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
			: system(system), entityID(entityID), metadata(metadata) {
		}

		void execute() override {
			if (!system || isCancelled()) return;

			try {
				bool success = system->RunUpscaling(entityID, metadata);
				markDone();
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in upscaling task: " << e.what() << std::endl;
				markDone();
			}
		}

	private:
		SDCPPSystem* system;
		EntityID entityID;
		nlohmann::json metadata;
	};

	// Implementation of task creation methods
	inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateInferenceTask(const EntityID entityID, const nlohmann::json& metadata) {
		return std::make_shared<InferenceTask>(this, entityID, metadata);
	}

	inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateConversionTask(EntityID entityID, const nlohmann::json& metadata) {
		return std::make_shared<ConvertTask>(this, entityID, metadata);
	}

	inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata) {
		return std::make_shared<Img2ImgTask>(this, entityID, metadata);
	}

	inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata) {
		return std::make_shared<UpscalingTask>(this, entityID, metadata);
	}

	// Implementation of core methods for generation

	// Run inference
	inline bool SDCPPSystem::RunInference(const EntityID entityID, const nlohmann::json& metadata) {
		sd_ctx_t* sd_context = nullptr;
		try {
			std::cout << "Starting inference for Entity " << entityID << std::endl;

			// Initialize Stable Diffusion context
			std::cout << "Initializing SD context..." << std::endl;
			sd_context = InitializeStableDiffusionContext(metadata);
			if (!sd_context) {
				throw std::runtime_error("Failed to initialize Stable Diffusion context!");
			}

			// Generate image
			sd_image_t* image = GenerateImage(sd_context, metadata);
			if (!image) {
				throw std::runtime_error("Failed to generate image!");
			}

			// Save the generated image
			SaveImage(image->data, image->width, image->height, image->channel, entityID, metadata);

			// Cleanup
			free(image);
			free_sd_ctx(sd_context);

			std::cout << "Inference completed for Entity " << entityID << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during inference: " << e.what() << std::endl;

			if (sd_context) {
				free_sd_ctx(sd_context);
			}
			return false;
		}
	}

	// Convert to GGUF
	inline bool SDCPPSystem::ConvertToGGUF(EntityID entityID, const nlohmann::json& metadata) {
		try {
			std::cout << "Starting conversion for Entity " << entityID << std::endl;

			std::string inputPath, vaePath;
			sd_type_t type = SD_TYPE_F16;

			// Extract model paths from metadata
			if (metadata.contains("components") && metadata["components"].is_array()) {
				for (const auto& comp : metadata["components"]) {
					if (comp.contains("Model")) {
						auto model = comp["Model"];
						if (model.contains("modelPath"))
							inputPath = model["modelPath"];
					}

					if (comp.contains("Vae")) {
						auto vae = comp["Vae"];
						if (vae.contains("modelPath"))
							vaePath = vae["modelPath"];
					}

					if (comp.contains("Sampler")) {
						auto sampler = comp["Sampler"];
						if (sampler.contains("current_type_method"))
							type = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
					}
				}
			}

			// Validate input path
			if (inputPath.empty()) {
				throw std::runtime_error("Input model path is empty");
			}

			// Create output path with type suffix
			std::filesystem::path inPath(inputPath);
			std::string outPath = inPath.parent_path().string() + "/" +
				inPath.stem().string() + "_" +
				std::string(type_method_items[type]) + ".gguf";

			// Perform conversion
			bool result;
			if (vaePath.empty()) {
				// Convert without VAE
				result = convert(inputPath.c_str(), nullptr, outPath.c_str(), type);
			}
			else {
				// Convert with VAE
				result = convert(inputPath.c_str(), vaePath.c_str(), outPath.c_str(), type);
			}

			if (!result) {
				throw std::runtime_error("Failed to convert Model: " + inputPath);
			}

			std::cout << "Successfully converted model to: " << outPath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during conversion: " << e.what() << std::endl;
			return false;
		}
	}

	inline bool SDCPPSystem::RunImg2Img(const EntityID entityID, const nlohmann::json& metadata) {
		sd_ctx_t* sd_context = nullptr;
		try {
			std::cout << "Starting img2img inference for Entity ID: " << entityID << std::endl;

			// Verify input image component exists and has data
			if (!mgr.HasComponent<InputImageComponent>(entityID)) {
				std::cerr << "Error: Entity " << entityID << " missing InputImageComponent" << std::endl;
				return false;
			}

			auto& inputComp = mgr.GetComponent<InputImageComponent>(entityID);

			if (!inputComp.imageData || inputComp.width <= 0 || inputComp.height <= 0) {
				std::cerr << "Error: Input image data is invalid for entity " << entityID << std::endl;
				return false;
			}

			std::cout << "Input image: " << inputComp.width << "x" << inputComp.height
				<< " with " << inputComp.channels << " channels" << std::endl;

			// Ensure output component exists
			if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
				auto& outputComp = mgr.AddComponent<OutputImageComponent>(entityID);
				std::cout << "Added missing OutputImageComponent to entity " << entityID << std::endl;

				// Set default output path and create directory if needed
				std::string outputDir = filePaths.defaultProjectPath + "/images";
				std::filesystem::create_directories(outputDir);

				outputComp.fileName = "img2img_" + std::to_string(entityID) + ".png";
				outputComp.filePath = outputDir;

				std::cout << "Set output path to: " << outputComp.filePath << "/" << outputComp.fileName << std::endl;
			}

			// Initialize Stable Diffusion context
			std::cout << "Initializing SD context..." << std::endl;
			sd_context = InitializeStableDiffusionContext(metadata);
			if (!sd_context) {
				throw std::runtime_error("Failed to initialize Stable Diffusion context!");
			}

			// Extract parameters from metadata
			std::string posPrompt = "", negPrompt = "";
			float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
			int latentWidth = inputComp.width, latentHeight = inputComp.height;
			int steps = 20, seed = -1, batchSize = 1;
			float denoiseStrength = 0.75f;
			sample_method_t sample_method = EULER;

			// Extract parameters from components array in metadata
			std::cout << "Extracting parameters from metadata..." << std::endl;
			if (metadata.contains("components") && metadata["components"].is_array()) {
				for (const auto& comp : metadata["components"]) {
					// Prompt component
					if (comp.contains("Prompt")) {
						auto prompt = comp["Prompt"];
						if (prompt.contains("posPrompt"))
							posPrompt = prompt["posPrompt"];
						if (prompt.contains("negPrompt"))
							negPrompt = prompt["negPrompt"];
						std::cout << "Found prompt: " << posPrompt << std::endl;
					}

					// ClipSkip component
					if (comp.contains("ClipSkip")) {
						auto clipSkipComp = comp["ClipSkip"];
						if (clipSkipComp.contains("clipSkip"))
							clipSkip = clipSkipComp["clipSkip"];
					}

					// Sampler component
					if (comp.contains("Sampler")) {
						auto sampler = comp["Sampler"];
						if (sampler.contains("cfg"))
							cfg = sampler["cfg"];
						if (sampler.contains("steps"))
							steps = sampler["steps"];
						if (sampler.contains("seed"))
							seed = sampler["seed"];
						if (sampler.contains("denoise"))
							denoiseStrength = sampler["denoise"];
						if (sampler.contains("current_sample_method"))
							sample_method = static_cast<sample_method_t>(sampler["current_sample_method"].get<int>());
					}

					// Guidance component
					if (comp.contains("Guidance")) {
						auto guidanceComp = comp["Guidance"];
						if (guidanceComp.contains("guidance"))
							guidance = guidanceComp["guidance"];
						if (guidanceComp.contains("eta"))
							eta = guidanceComp["eta"];
					}

					// Latent component
					if (comp.contains("Latent")) {
						auto latent = comp["Latent"];
						if (latent.contains("latentWidth"))
							latentWidth = latent["latentWidth"];
						if (latent.contains("latentHeight"))
							latentHeight = latent["latentHeight"];
						if (latent.contains("batchSize"))
							batchSize = latent["batchSize"];
					}
				}
			}

			// Prepare input image struct
			auto* data = Utils::ImageUtils::LoadImageData(inputComp.filePath, inputComp.width, inputComp.height, inputComp.channels);
			std::cout << "Preparing input image..." << std::endl;
			sd_image_t init_image = {
				static_cast<uint32_t>(inputComp.width),
				static_cast<uint32_t>(inputComp.height),
				static_cast<uint32_t>(inputComp.channels),
				reinterpret_cast<uint8_t*>(data)
			};

			// Prepare empty mask image (can be enhanced later for inpainting)
			sd_image_t mask_image = { 0 };

			std::cout << "Calling img2img with denoise strength: " << denoiseStrength << std::endl;

			// Call the img2img function with all parameters
			sd_image_t* result_image = img2img(
				sd_context,
				init_image,
				mask_image,
				posPrompt.c_str(),
				negPrompt.c_str(),
				clipSkip,
				cfg,
				guidance,
				eta,
				latentWidth,
				latentHeight,
				sample_method,
				steps,
				denoiseStrength,
				seed,
				batchSize,
				nullptr,  // control_cond
				0.0f,     // control_strength
				0.0f,     // style_strength
				false,    // normalize_input
				"",       // input_id_images_path
				nullptr,  // skip_layers
				0,        // skip_layers_count
				0.0f,     // slg_scale
				0.0f,     // skip_layer_start
				1.0f      // skip_layer_end
			);

			if (!result_image) {
				throw std::runtime_error("Failed to generate image!");
			}

			std::cout << "Image generated successfully. Saving result..." << std::endl;

			// Save the generated image with metadata
			SaveImage(result_image->data, result_image->width, result_image->height,
				result_image->channel, entityID, metadata);

			// Free the result image
			free(result_image);

			// Free the SD context
			free_sd_ctx(sd_context);

			std::cout << "Img2Img inference completed for Entity " << entityID << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during img2img inference: " << e.what() << std::endl;

			if (sd_context) {
				free_sd_ctx(sd_context);
			}

			return false;
		}
	}

	inline bool SDCPPSystem::RunUpscaling(EntityID entityID, const nlohmann::json& metadata) {
		upscaler_ctx_t* upscaler_context = nullptr;
		try {
			std::cout << "Starting upscaling for Entity " << entityID << std::endl;

			std::string modelPath;
			uint32_t upscaleFactor = 2;
			bool preserveAspectRatio = true;
			int n_threads = 4;

			// Input image component
			if (!mgr.HasComponent<InputImageComponent>(entityID)) {
				return false;
			}
			auto& inputComp = mgr.GetComponent<InputImageComponent>(entityID);

			// Extract parameters from metadata
			if (metadata.contains("components") && metadata["components"].is_array()) {
				for (const auto& comp : metadata["components"]) {
					// Esrgan component (upscaler model)
					if (comp.contains("Esrgan")) {
						auto esrgan = comp["Esrgan"];
						if (esrgan.contains("modelPath"))
							modelPath = esrgan["modelPath"];
						else if (esrgan.contains("modelName") && !esrgan["modelName"].get<std::string>().empty())
							modelPath = filePaths.upscaleDir + "/" + esrgan["modelName"].get<std::string>();

						if (esrgan.contains("upscaleFactor"))
							upscaleFactor = esrgan["upscaleFactor"];

						if (esrgan.contains("preserveAspectRatio"))
							preserveAspectRatio = esrgan["preserveAspectRatio"];
					}



					// Sampler component for threads count
					if (comp.contains("Sampler")) {
						if (comp["Sampler"].contains("n_threads"))
							n_threads = comp["Sampler"]["n_threads"];
					}
				}
			}

			// Validate parameters
			if (modelPath.empty()) {
				throw std::runtime_error("ESRGAN model path is empty!");
			}

			// Initialize upscaler context
			upscaler_context = new_upscaler_ctx(modelPath.c_str(), n_threads);
			if (!upscaler_context) {
				throw std::runtime_error("Failed to initialize upscaler context!");
			}

			// Create input image
			sd_image_t input_image = {
				static_cast<uint32_t>(inputComp.width),
				static_cast<uint32_t>(inputComp.height),
				static_cast<uint32_t>(inputComp.channels),
				inputComp.imageData
			};

			// Perform upscaling
			sd_image_t upscaled_image = upscale(upscaler_context, input_image, upscaleFactor);

			// Save the upscaled image
			SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
				upscaled_image.channel, entityID, metadata);

			// Cleanup upscaler context
			free_upscaler_ctx(upscaler_context);

			std::cout << "Upscaling completed for Entity " << entityID << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Exception during upscaling: " << e.what() << std::endl;

			if (upscaler_context) {
				free_upscaler_ctx(upscaler_context);
			}

			return false;
		}
	}

} // namespace ECS