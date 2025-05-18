#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "ImageUtils.hpp"
#include "ImageSystem.hpp"
#include "SDCPPComponents.h"
#include "SDcppUtils.hpp"
#include "SDCPPTasks.hpp"
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
			std::string fullPath = "";
			std::shared_ptr<Utils::Task> task;
			TaskType taskType;
		};

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
				// Create queue item
				std::lock_guard<std::mutex> lock(queueMutex);
				QueueItem item;
				item.entityID = entityID;
				item.processing = false;
				item.taskType = taskType;

				// Check if we need to generate a random seed
				if (taskType == TaskType::Inference || taskType == TaskType::Img2Img) {
					// Access the sampler component
					if (mgr.HasComponent<SamplerComponent>(entityID)) {
						auto& samplerComp = mgr.GetComponent<SamplerComponent>(entityID);

						// Generate random seed if needed
						if (samplerComp.seed < 0) {
							uint64_t newSeed = Utils::SDCPPUtils::generateRandomSeed();
							samplerComp.seed = static_cast<int>(newSeed);
							std::cout << "Generated random seed: " << samplerComp.seed << std::endl;
						}
					}
				}

				// Serialize entity to metadata
				item.metadata = mgr.SerializeEntity(entityID);
				std::cout << "Successfully serialized entity " << entityID << std::endl;

				taskQueue.push_back(item);

				std::cout << "Entity " << entityID << " queued for processing. New queue size: " << taskQueue.size() << std::endl;
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
			// TODO: check between steps to cancel
		}

		void ClearQueue() {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (taskQueue.empty()) {
				return;
			}
			// First cancel any active tasks
			for (auto& item : taskQueue) {
				if (item.task) {
					item.task->cancel();
				}
			}

			// Get reference to ImageSystem if available
			auto imageSystem = mgr.GetSystem<ImageSystem>();

			// Clear non-processing tasks
			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				if (!it->processing) {
					EntityID entityId = it->entityID;
					it = taskQueue.erase(it);

					try {
						// Use ImageSystem to properly remove image if applicable
						if (imageSystem && mgr.HasComponent<ImageComponent>(entityId)) {
							imageSystem->RemoveImage(entityId);
						}
						else if (mgr.GetEntitiesSignatures().count(entityId)) {
							mgr.DestroyEntity(entityId);
						}
					}
					catch (const std::exception& e) {
						std::cerr << "Error in ClearQueue: " << e.what() << std::endl;
					}
				}
				else {
					++it;
				}
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

		// Helper methods to create tasks
		std::shared_ptr<Utils::Task> CreateInferenceTask(const EntityID entityID, const nlohmann::json& metadata, std::string fullPath) {
			return std::make_shared<InferenceTask>(metadata, fullPath);
		}

		std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID, const nlohmann::json& metadata) {
			return std::make_shared<ConvertTask>(metadata);
		}

		std::shared_ptr<Utils::Task> CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata, std::string fullPath) {
			return std::make_shared<Img2ImgTask>(metadata, fullPath);
		}

		std::shared_ptr<Utils::Task> CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata, std::string fullPath) {
			return std::make_shared<UpscalingTask>(metadata, fullPath);
		}

		// Queue processing methods
		void ProcessQueues() {
			std::lock_guard<std::mutex> lock(queueMutex);

			if (pauseWorker) {
				return;
			}

			if (taskQueue.empty()) {
				return;
			}

			auto& item = taskQueue.front();

			// Process only if it's not already processing and we have capacity
			if (!item.processing && activeTasks < 1) {
				// Create a task based on the task type
				if (mgr.HasComponent<OutputImageComponent>(item.entityID))
				{
					auto& output = mgr.GetComponent<OutputImageComponent>(item.entityID);
					item.fullPath = Utils::PngMetadata::CreateUniqueFilename(output.fileName, output.filePath);
				}
				switch (item.taskType) {
				case TaskType::Inference:
					item.task = CreateInferenceTask(item.entityID, item.metadata, item.fullPath);
					break;
				case TaskType::Conversion:
					item.task = CreateConversionTask(item.entityID, item.metadata);
					break;
				case TaskType::Img2Img:
					item.task = CreateImg2ImgTask(item.entityID, item.metadata, item.fullPath);
					break;
				case TaskType::Upscaling:
					item.task = CreateUpscalingTask(item.entityID, item.metadata, item.fullPath);
					break;
				default:
					return; // Invalid task type, don't process
				}

				item.processing = true;
				activeTasks++;

				// Add to thread pool
				threadPool.addTask(item.task);
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

					// Decrease active tasks count
					activeTasks--;

					// Process the completed task before erasing
					ProcessCompletedTask(entityID);

					// Now erase the item
					it = taskQueue.erase(it);
				}
				else {
					++it;
				}
			}
		}

		void ProcessCompletedTask(const EntityID entityID) {
			try {
				if (!mgr.GetEntitiesSignatures().count(entityID)) {
					std::cerr << "Entity no longer exists: " << entityID << std::endl;
					return;
				}

				if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
					std::cerr << "Missing OutputImageComponent for entity: " << entityID << std::endl;
					return;
				}

				auto& outputComp = mgr.GetComponent<OutputImageComponent>(entityID);

				if (outputComp.filePath.empty()) {
					std::cerr << "No filepath in OutputImageComponent for entity: " << entityID << std::endl;
					return;
				}

				auto imageSystem = mgr.GetSystem<ImageSystem>();
				if (!imageSystem) {
					std::cerr << "ImageSystem not found" << std::endl;
					return;
				}

				// Create/update Image component
				if (!mgr.HasComponent<ImageComponent>(entityID)) {
					auto& imageComp = mgr.AddComponent<ImageComponent>(entityID);
					// The ImageSystem::SetImage will handle initializing and loading
				}

				// Use the ImageSystem to set the image path and load it
				// This will handle cleaning up any existing image data
				std::string fullPath = "";
				if (!taskQueue.empty() && taskQueue.front().entityID == entityID) {
					fullPath = taskQueue.front().fullPath;
				}
				else {
					fullPath = outputComp.filePath;
				}

				if (std::filesystem::exists(fullPath)) {
					imageSystem->SetImage(entityID, fullPath);
					std::cout << "Image loaded successfully for entity " << entityID << std::endl;
				}
				else {
					std::cerr << "Failed to find output image file: " << fullPath << std::endl;
					return;
				}

				// Clean up unnecessary components, leaving only the ImageComponent
				std::vector<ComponentTypeID> componentTypes = mgr.GetEntityComponents(entityID);
				for (const auto& componentId : componentTypes) {
					std::string componentName = mgr.GetComponentNameById(componentId);
					if (componentName != "Image") {
						// Note: We don't need to manually clean up image data here 
						// because the ImageSystem did that in SetImage
						mgr.RemoveComponentById(entityID, componentId);
					}
				}

				std::cout << "Successfully processed completed task for entity " << entityID << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Error processing completed task: " << e.what() << std::endl;
				try {
					// Get an ImageSystem reference if available
					auto imageSystem = mgr.GetSystem<ImageSystem>();
					if (imageSystem) {
						// Use ImageSystem::RemoveImage which properly cleans up the image resources
						imageSystem->RemoveImage(entityID);
					}
					else {
						mgr.DestroyEntity(entityID);
					}
				}
				catch (const std::exception& e2) {
					std::cerr << "Error destroying entity: " << e2.what() << std::endl;
				}
			}
		}	};

} // namespace ECS