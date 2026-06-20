#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "rng.hpp"

#include "SDCPPComponents.h"
#include "Components.h"

#include "Txt2Img.hpp"
#include "Img2Img.hpp"
#include "Img2Vid.hpp"
#include "Edit.hpp"
#include "Upscaling.hpp"
#include "Conversion.hpp"
#include "PngMetadataUtils.hpp"

#include "ImageSystem.hpp"
#include "VideoSystem.hpp"

#include "ContextUtils.hpp"

// Standard includes
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPool.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <optional>
#include <future>
#include <thread>

namespace ECS {
	class SDCPPSystem : public BaseSystem {
	public:
		enum class TaskType {
			Inference,     // txt2img
			Conversion,    // GGUF conversion
			Img2Img,       // img2img
			Img2Vid,       // img2vid
			Edit,          // edit
			Upscaling      // upscaling
		};

		// Simplified queue item for public interface
		struct QueueItem {
			EntityID entityID = 0;
			bool processing = false;
			TaskType taskType;

			// Default constructor
			QueueItem() = default;

			// Copy constructor
			QueueItem(const QueueItem& other) = default;

			// Assignment operator
			QueueItem& operator=(const QueueItem& other) = default;
		};

		// Internal tracking structure with proper context management
		struct TaskData {
			EntityID entityID = 0;
			bool processing = false;
			TaskType taskType;
			nlohmann::json metadata;
			std::string fullPath;
			std::future<bool> result;

			// Contexts - only one will be used depending on task type
			union {
				sd_ctx_t* sdContext;
				upscaler_ctx_t* upscalerContext;
			};

			bool contextAcquired = false;
			bool modelLoading = false;

			// Store context type for proper cleanup
			enum ContextType {
				NoContext,      // Conversion tasks
				SDContext,      // Inference/Img2Img/Img2Vid/Edit tasks
				UpscalerContext // Upscaling tasks
			} contextType = NoContext;

			// Default constructor
			TaskData() : sdContext(nullptr), contextType(NoContext) {}

			// Move constructor
			TaskData(TaskData&& other) noexcept
				: entityID(other.entityID)
				, processing(other.processing)
				, taskType(other.taskType)
				, metadata(std::move(other.metadata))
				, fullPath(std::move(other.fullPath))
				, result(std::move(other.result))
				, contextAcquired(other.contextAcquired)
				, modelLoading(other.modelLoading)
				, contextType(other.contextType) {

				// Move the appropriate context pointer
				switch (contextType) {
				case SDContext:
					sdContext = other.sdContext;
					other.sdContext = nullptr;
					break;
				case UpscalerContext:
					upscalerContext = other.upscalerContext;
					other.upscalerContext = nullptr;
					break;
				case NoContext:
				default:
					sdContext = nullptr;
					break;
				}
			}

			// Move assignment operator
			TaskData& operator=(TaskData&& other) noexcept {
				if (this != &other) {
					// Clean up existing context first
					CleanupContext();

					entityID = other.entityID;
					processing = other.processing;
					taskType = other.taskType;
					metadata = std::move(other.metadata);
					fullPath = std::move(other.fullPath);
					result = std::move(other.result);
					contextAcquired = other.contextAcquired;
					modelLoading = other.modelLoading;
					contextType = other.contextType;

					// Move the appropriate context pointer
					switch (contextType) {
					case SDContext:
						sdContext = other.sdContext;
						other.sdContext = nullptr;
						break;
					case UpscalerContext:
						upscalerContext = other.upscalerContext;
						other.upscalerContext = nullptr;
						break;
					case NoContext:
					default:
						sdContext = nullptr;
						break;
					}
				}
				return *this;
			}

			// Delete copy operations
			TaskData(const TaskData&) = delete;
			TaskData& operator=(const TaskData&) = delete;

			// Destructor
			~TaskData() {
				CleanupContext();
			}

			// Helper to clean up context
			void CleanupContext() {
				switch (contextType) {
				case SDContext:
					if (sdContext) {
						std::cout << "DEBUG: Cleaning up SD context for entity " << entityID
							<< " (acquired: " << contextAcquired
							<< ", loading: " << modelLoading << ")" << std::endl;

						if (contextAcquired && !modelLoading) {
							// We properly acquired this context, release it normally
							Utils::SDContextManager::ReleaseContext(sdContext);
						}
						else {
							// Context wasn't properly acquired or is still loading, force free it
							Utils::SDContextManager::ForceFreeContext(sdContext);
						}
						sdContext = nullptr;
					}
					break;
				case UpscalerContext:
					if (upscalerContext) {
						std::cout << "DEBUG: Cleaning up upscaler context for entity " << entityID << std::endl;
						free_upscaler_ctx(upscalerContext);
						upscalerContext = nullptr;
					}
					break;
				case NoContext:
				default:
					// Nothing to clean up
					break;
				}
				contextType = NoContext;
				contextAcquired = false;
				modelLoading = false;
			}
		};

		// Constructor
		SDCPPSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr)
			, pauseWorker(false)
			, hasActiveTask(false)
			, clearRequested(false)
		{
			sysName = "SDCPPSystem";
			std::cout << "[SDCPPSystem] Initialized with smart context management" << std::endl;
		}

		// Destructor
		~SDCPPSystem() {
			Shutdown();
		}

		void Shutdown() {
			std::cout << "[SDCPPSystem] Shutting down system..." << std::endl;

			// Signal shutdown
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				shuttingDown = true;
				pauseWorker = true;
			}

			// Stop current task
			StopCurrentTask();

			// Clear queue and clean up contexts
			ClearQueue();

			// Wait for all threads to complete
			if (workerThread.joinable()) {
				workerThread.join();
			}

			// Shutdown thread pools
			auto& diffusionPool = Utils::ThreadPoolManager::getInstance().getDiffusionPool();
			auto startTime = std::chrono::steady_clock::now();
			while (std::chrono::steady_clock::now() - startTime < std::chrono::seconds(5)) {
				if (diffusionPool.getActiveCount() == 0 && diffusionPool.getQueueSize() == 0) {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			// Clear all contexts one more time
			Utils::SDContextManager::ClearAllContexts();

			std::cout << "[SDCPPSystem] Shutdown complete" << std::endl;
		}

		void TerminateImmediately() {
			std::cout << "[SDCPPSystem] Immediate termination requested" << std::endl;

			// Signal shutdown
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				shuttingDown = true;
				pauseWorker = true;
				terminateFlag = true;
			}

			// Clear the task queue
			ClearQueue();

			// Clear all contexts
			Utils::SDContextManager::ClearAllContexts();

			// Terminate the thread pool immediately
			Utils::ThreadPoolManager::getInstance().getDiffusionPool().terminate();

			std::cout << "[SDCPPSystem] Immediate termination complete" << std::endl;
		}

		void QueueTask(const EntityID entityID, const TaskType taskType) {
			std::cout << "[QueueTask] Starting for entity: " << entityID << std::endl;

			// Validate entity exists
			if (!mgr.IsEntityValid(entityID)) {
				std::cerr << "[QueueTask] ERROR: Entity " << entityID << " is not valid!" << std::endl;
				return;
			}

			try {
				std::lock_guard<std::mutex> lock(queueMutex);

				// Don't accept new tasks during shutdown
				if (shuttingDown) {
					std::cerr << "Cannot queue task during shutdown" << std::endl;
					return;
				}

				std::cout << "Processing entity: " << entityID << std::endl;
				std::cout << "Task type: " << static_cast<int>(taskType) << std::endl;

				TaskData taskData;
				taskData.entityID = entityID;
				taskData.processing = false;
				taskData.taskType = taskType;
				taskData.modelLoading = false;
				taskData.contextType = TaskData::NoContext;

				// Set seed for tasks that need it
				if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
					taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {

					if (mgr.HasComponent<SamplerComponent>(entityID)) {
						auto& samplerComp = mgr.GetComponent<SamplerComponent>(entityID);
						if (samplerComp.seed < 0) {
							uint64_t newSeed = Utils::generateRandomSeed();
							samplerComp.seed = static_cast<int>(newSeed);
							std::cout << "Generated random seed: " << samplerComp.seed << std::endl;
						}
					}
				}

				try {
					taskData.metadata = mgr.SerializeEntity(entityID);
					std::cout << "Serialization successful" << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "Serialization failed: " << e.what() << std::endl;
					return;
				}

				taskQueue.push_back(std::move(taskData));

				std::cout << "Entity " << entityID << " queued for processing. Queue position: " << taskQueue.size() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in QueueTask: " << e.what() << std::endl;
			}
		}

		void Update(const float deltaT) override {
			if (shuttingDown) {
				return;
			}

			// Handle clear request
			if (clearRequested) {
				HandleClearRequest();
				clearRequested = false;
			}

			// Process queues - start next task if no task is currently running
			ProcessQueues();

			// Update status of running task
			CheckTaskCompletion();

			// Check for model loading failures
			CheckModelLoadingFailures();
		}

		void RemoveFromQueue(const size_t index) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (index < taskQueue.size() && !taskQueue[index].processing) {
				EntityID entityID = taskQueue[index].entityID;

				// Clean up context
				taskQueue[index].CleanupContext();

				taskQueue.erase(taskQueue.begin() + index);

				// Add to deferred cleanup list
				entitiesNeedingCleanup.push_back(entityID);
			}
		}

		void MoveInQueue(const size_t fromIndex, const size_t toIndex) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size())
				return;
			if (taskQueue[fromIndex].processing)
				return;

			TaskData task = std::move(taskQueue[fromIndex]);
			taskQueue.erase(taskQueue.begin() + fromIndex);
			taskQueue.insert(taskQueue.begin() + toIndex, std::move(task));
		}

		// Return copyable queue items for UI display
		std::vector<QueueItem> GetQueueSnapshot() {
			std::lock_guard<std::mutex> lock(queueMutex);
			std::vector<QueueItem> result;
			result.reserve(taskQueue.size());

			for (const auto& task : taskQueue) {
				QueueItem item;
				item.entityID = task.entityID;
				item.processing = task.processing;
				item.taskType = task.taskType;
				result.push_back(item);
			}

			return result;
		}

		void StopCurrentTask() {
			std::lock_guard<std::mutex> lock(queueMutex);

			if (hasActiveTask && activeThreadId != std::thread::id{}) {
				std::cout << "Attempting to terminate active task on thread: " << activeThreadId << std::endl;
				// Set termination flag
				terminateFlag = true;
			}
			else {
				std::cout << "No active task to terminate" << std::endl;
			}
		}

		void ClearQueue() {
			std::cout << "Queue clear requested" << std::endl;
			clearRequested = true;
		}

		void PauseWorker() {
			std::lock_guard<std::mutex> lock(queueMutex);
			pauseWorker = true;
			std::cout << "Worker paused. Current task will continue but no new tasks will be started." << std::endl;
		}

		void ResumeWorker() {
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				pauseWorker = false;
			}
			std::cout << "Worker resumed. New tasks will now be processed." << std::endl;
		}

		// Thread pool stats
		size_t GetNumThreads() const {
			return Utils::ThreadPoolManager::getInstance().getDiffusionPool().size();
		}

		size_t GetQueuedTaskCount() const {
			return Utils::ThreadPoolManager::getInstance().getDiffusionPool().getQueueSize();
		}

		size_t GetActiveTaskCount() const {
			return Utils::ThreadPoolManager::getInstance().getDiffusionPool().getActiveCount();
		}

		bool HasActiveTask() const {
			std::lock_guard<std::mutex> lock(queueMutex);
			return hasActiveTask;
		}

		std::thread::id GetActiveThreadId() const {
			std::lock_guard<std::mutex> lock(queueMutex);
			return activeThreadId;
		}

		size_t GetQueueSize() const {
			std::lock_guard<std::mutex> lock(queueMutex);
			return taskQueue.size();
		}

		// Get the last generated video entity ID for VideoDiffusionView
		EntityID GetLastGeneratedVideo() const {
			std::lock_guard<std::mutex> lock(queueMutex);
			return lastGeneratedVideoEntity;
		}

		// Context Management Methods
		void ClearAllSDContexts() {
			std::cout << "[SDCPPSystem] Clearing all SD contexts..." << std::endl;
			Utils::SDContextManager::ClearAllContexts();
		}

		void ListSDContexts() {
			std::cout << "[SDCPPSystem] Listing all cached SD contexts:" << std::endl;
			Utils::SDContextManager::ListCachedContexts();
		}

		// Get SD context stats
		std::tuple<size_t, size_t, size_t> GetSDContextStats() {
			size_t totalCached = 0, inUse = 0, available = 0;
			Utils::SDContextManager::GetCacheStats(totalCached, inUse, available);
			return std::make_tuple(totalCached, inUse, available);
		}

		// Get model loading stats
		std::tuple<size_t, size_t> GetModelLoadingStats() {
			size_t loadingCount = 0, failedCount = 0;
			Utils::SDContextManager::GetLoadingStats(loadingCount, failedCount);
			return std::make_tuple(loadingCount, failedCount);
		}

		// Force reload model for next task
		void ForceModelReload() {
			std::lock_guard<std::mutex> lock(queueMutex);
			forceModelReload = true;
			std::cout << "[SDCPPSystem] Model reload forced for next task" << std::endl;
		}

		// ================================================
		// Model Management Controls
		// ================================================

		// Set maximum number of models to cache
		void SetMaxModelCache(size_t maxModels) {
			Utils::SDContextManager::SetMaxCacheSize(maxModels);
			std::cout << "[SDCPPSystem] Max model cache set to: " << maxModels << std::endl;
		}

		size_t GetMaxModelCache() const {
			return Utils::SDContextManager::GetMaxCacheSize();
		}

		// Unload specific model (safely - only if not in use)
		void UnloadModel(const std::string& modelPath) {
			std::cout << "[SDCPPSystem] Attempting to unload model: " << modelPath << std::endl;

			// Check if any task is currently using this model
			std::lock_guard<std::mutex> lock(queueMutex);
			bool modelInUse = false;

			for (const auto& task : taskQueue) {
				if (task.processing && task.sdContext) {
					// This task is currently running - we can't unload its model
					modelInUse = true;
					break;
				}
			}

			if (!modelInUse) {
				Utils::SDContextManager::UnloadSpecificModel(modelPath);
				std::cout << "[SDCPPSystem] Model unloaded: " << modelPath << std::endl;
			}
			else {
				std::cout << "[SDCPPSystem] WARNING: Cannot unload model " << modelPath
					<< " - currently in use by active task" << std::endl;
			}
		}

		// Unload all models (safely - only if no tasks are running)
		void UnloadAllModels() {
			std::cout << "[SDCPPSystem] Attempting to unload all models..." << std::endl;

			std::lock_guard<std::mutex> lock(queueMutex);
			if (!hasActiveTask) {
				Utils::SDContextManager::UnloadAllModels();
				std::cout << "[SDCPPSystem] All models unloaded" << std::endl;
			}
			else {
				std::cout << "[SDCPPSystem] WARNING: Cannot unload all models - tasks are currently running" << std::endl;
			}
		}

		// Force unload idle models
		void ForceUnloadIdleModels() {
			std::cout << "[SDCPPSystem] Force unloading idle models..." << std::endl;

			// Get all loaded models
			auto loadedModels = Utils::SDContextManager::GetLoadedModels();

			if (loadedModels.empty()) {
				std::cout << "[SDCPPSystem] No models currently loaded" << std::endl;
				return;
			}

			std::cout << "[SDCPPSystem] Currently loaded models:" << std::endl;
			for (const auto& model : loadedModels) {
				std::cout << "  - " << model << std::endl;
			}

			// Unload ALL models
			Utils::SDContextManager::UnloadAllModels();
			std::cout << "[SDCPPSystem] All models unloaded" << std::endl;
		}

		// Check if a model is loaded
		bool IsModelLoaded(const std::string& modelPath) const {
			return Utils::SDContextManager::IsModelLoaded(modelPath);
		}

		// Get list of loaded models
		std::vector<std::string> GetLoadedModels() const {
			return Utils::SDContextManager::GetLoadedModels();
		}

		// Get current model cache stats
		std::tuple<size_t, size_t, size_t> GetModelCacheStats() const {
			size_t totalCached = 0, inUse = 0, available = 0;
			Utils::SDContextManager::GetCacheStats(totalCached, inUse, available);
			return std::make_tuple(totalCached, inUse, available);
		}

	private:
		std::vector<TaskData> taskQueue;
		std::atomic<bool> pauseWorker;
		std::atomic<bool> shuttingDown{ false };
		std::atomic<bool> terminateFlag{ false };
		std::atomic<bool> clearRequested{ false };
		std::atomic<bool> forceModelReload{ false };
		std::vector<EntityID> entitiesNeedingCleanup;
		mutable std::mutex queueMutex;
		EntityID lastGeneratedVideoEntity{ 0 };

		std::thread workerThread;

		// Single task tracking
		bool hasActiveTask{ false };
		std::thread::id activeThreadId{};

		// Helper to determine which context type a task needs
		TaskData::ContextType GetRequiredContextType(TaskType taskType) {
			switch (taskType) {
			case TaskType::Inference:
			case TaskType::Img2Img:
			case TaskType::Img2Vid:
			case TaskType::Edit:
				return TaskData::SDContext;
			case TaskType::Upscaling:
				return TaskData::UpscalerContext;
			case TaskType::Conversion:
			default:
				return TaskData::NoContext;
			}
		}

		// Helper to check if a task is ready to be processed
		bool IsTaskReadyForProcessing(TaskData& task) {
			switch (task.contextType) {
			case TaskData::SDContext:
				// For SD tasks, check if context is loaded AND not loading
				if (task.sdContext != nullptr && !task.modelLoading) {
					return true; // Context already loaded
				}

				if (task.modelLoading) {
					// Check if loading is complete
					sd_ctx_t* loadedContext = Utils::SDContextManager::TryGetLoadedContext(task.metadata);
					if (loadedContext) {
						task.sdContext = loadedContext;
						task.modelLoading = false;
						task.contextAcquired = true;
						return true;
					}
					return false; // Still loading
				}

				// Not loading yet, start loading
				return false;

			case TaskData::UpscalerContext:
				// Upscaler contexts are created synchronously, so if we have it, we're ready
				return task.upscalerContext != nullptr;

			case TaskData::NoContext:
				// Conversion tasks are always ready
				return true;

			default:
				return false;
			}
		}

		// Get or create context for a task
		bool AcquireContextForTask(TaskData& task) {
			switch (task.contextType) {
			case TaskData::SDContext: {
				// Check if force reload is requested
				if (forceModelReload) {
					// Force creation of new context
					task.sdContext = Utils::SDContextManager::CreateNewContext(task.metadata);
					task.modelLoading = false;
					forceModelReload = false;
					task.contextAcquired = (task.sdContext != nullptr);
					if (!task.contextAcquired) {
						std::cerr << "Force reload failed for entity " << task.entityID << std::endl;
					}
					return task.contextAcquired;
				}

				// Try to get or create context (async)
				task.sdContext = Utils::SDContextManager::GetOrCreateContext(task.metadata);

				// If context is nullptr, it means it's loading asynchronously
				if (!task.sdContext) {
					task.modelLoading = true;
					std::cout << "Model is loading asynchronously for entity " << task.entityID << std::endl;
					return false; // Not ready yet
				}

				task.modelLoading = false;
				task.contextAcquired = true;
				return true;
			}

			case TaskData::UpscalerContext: {
				// Upscaler context is created synchronously
				task.upscalerContext = CreateUpscalerContext(task.metadata);
				task.contextAcquired = (task.upscalerContext != nullptr);
				if (!task.contextAcquired) {
					std::cerr << "Failed to create upscaler context for entity " << task.entityID << std::endl;
				}
				return task.contextAcquired;
			}

			case TaskData::NoContext:
			default:
				// No context needed (Conversion tasks)
				task.contextAcquired = true;
				return true;
			}
		}

		// Create upscaler context from metadata (updated API)
		upscaler_ctx_t* CreateUpscalerContext(const nlohmann::json& metadata) {
			try {
				std::string esrganPath = "";
				bool direct = false;
				int n_threads = 4;
				int tile_size = 64;
				std::string backend = "";
				std::string params_backend = "";

				// Parse metadata to extract upscaler parameters
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						if (comp.contains("Esrgan")) {
							nlohmann::json esrganData = comp["Esrgan"];

							if (esrganData.contains("modelPath") && !esrganData["modelPath"].is_null()) {
								esrganPath = esrganData["modelPath"].get<std::string>();
							}

							if (esrganData.contains("direct")) {
								direct = esrganData["direct"].get<bool>();
							}
							if (esrganData.contains("n_threads")) {
								n_threads = esrganData["n_threads"].get<int>();
							}
							if (esrganData.contains("tile_size")) {
								tile_size = esrganData["tile_size"].get<int>();
							}
							if (esrganData.contains("backend") && !esrganData["backend"].is_null()) {
								backend = esrganData["backend"].get<std::string>();
							}
							if (esrganData.contains("params_backend") && !esrganData["params_backend"].is_null()) {
								params_backend = esrganData["params_backend"].get<std::string>();
							}
						}

						if (comp.contains("Sampler")) {
							nlohmann::json samplerData = comp["Sampler"];
							if (samplerData.contains("n_threads")) {
								n_threads = samplerData["n_threads"].get<int>();
							}
						}
					}
				}

				if (esrganPath.empty()) {
					std::cerr << "No ESRGAN model path specified for upscaling" << std::endl;
					return nullptr;
				}

				std::cout << "Creating upscaler context with path: " << esrganPath << std::endl;
				// Updated API: new_upscaler_ctx(path, direct, n_threads, tile_size, backend, params_backend)
				return new_upscaler_ctx(esrganPath.c_str(),
					direct,
					n_threads,
					tile_size,
					backend.empty() ? nullptr : backend.c_str(),
					params_backend.empty() ? nullptr : params_backend.c_str());
			}
			catch (const std::exception& e) {
				std::cerr << "Error creating upscaler context: " << e.what() << std::endl;
				return nullptr;
			}
		}

		// Check model loading failures
		void CheckModelLoadingFailures() {
			std::lock_guard<std::mutex> lock(queueMutex);

			// Check all non-processing tasks that are marked as loading
			for (auto& task : taskQueue) {
				if (!task.processing && task.modelLoading && task.contextType == TaskData::SDContext) {
					// Check if loading has completed (successfully or failed)
					if (!Utils::SDContextManager::IsContextLoading(task.metadata)) {
						// Loading is no longer in progress, check result
						sd_ctx_t* loadedContext = Utils::SDContextManager::TryGetLoadedContext(task.metadata);

						if (loadedContext == nullptr) {
							// Loading failed for this task
							std::cerr << "Model loading failed for entity " << task.entityID << std::endl;

							// Mark for cleanup
							task.modelLoading = false;
							task.contextAcquired = false;
						}
						else {
							// Loading succeeded
							task.sdContext = loadedContext;
							task.modelLoading = false;
							task.contextAcquired = true;
							std::cout << "Model loading completed for entity " << task.entityID << std::endl;
						}
					}
				}
			}
		}

		void LoadImageViaImageSystem(EntityID targetEntity, const std::string& filePath) {
			if (auto imageSystem = mgr.GetSystem<ImageSystem>()) {
				// Use the existing entity, don't create a new one
				if (!mgr.HasComponent<ImageComponent>(targetEntity)) {
					mgr.AddComponent<ImageComponent>(targetEntity);
				}

				// Load image directly to the target entity
				imageSystem->SetImage(targetEntity, filePath);

				std::cout << "[SDCPPSystem] Set image on entity " << targetEntity
					<< " from " << filePath << std::endl;
			}
			else {
				std::cerr << "[SDCPPSystem] ImageSystem not available!" << std::endl;
			}
		}

		void LoadVideoViaVideoSystem(const std::string& filePath) {
			if (auto videoSystem = mgr.GetSystem<VideoSystem>()) {
				// Create a NEW entity for the generated video
				EntityID videoEntity = mgr.AddNewEntity();
				mgr.AddComponent<OutputVideoComponent>(videoEntity);

				// Set the video path
				auto& videoComp = mgr.GetComponent<OutputVideoComponent>(videoEntity);
				videoComp.filePath = filePath;
				videoComp.fileName = std::filesystem::path(filePath).filename().string();

				// Use VideoSystem to load the video
				videoSystem->SetVideo(videoEntity, filePath);

				// Store the last generated video entity
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					lastGeneratedVideoEntity = videoEntity;
				}

				std::cout << "[SDCPPSystem] Created new video entity " << videoEntity
					<< " and queued video load from " << filePath << std::endl;
			}
			else {
				std::cerr << "[SDCPPSystem] VideoSystem not available!" << std::endl;
			}
		}

		void HandleClearRequest() {
			std::lock_guard<std::mutex> lock(queueMutex);
			std::cout << "Clearing queue with " << taskQueue.size() << " items" << std::endl;

			// Clean up contexts for all non-processing tasks
			for (auto& task : taskQueue) {
				if (!task.processing) {
					task.CleanupContext();
				}
			}

			// Remove all non-processing tasks
			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				if (!it->processing) {
					it = taskQueue.erase(it);
				}
				else {
					++it;  // Keep active tasks
				}
			}

			std::cout << "Queue cleared. " << taskQueue.size() << " items remaining (active)" << std::endl;
		}

		// Task wrappers for different context types
		template<typename Func, typename... Args>
		auto CreateTaskWrapper(EntityID entityID, Func&& func, Args&&... args) {
			return[this, entityID, func = std::forward<Func>(func), args...]() -> bool {
				// Set thread tracking info
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					hasActiveTask = true;
					activeThreadId = std::this_thread::get_id();
					terminateFlag = false;
				}

				std::cout << "Task started for entity " << entityID << " on thread " << std::this_thread::get_id() << std::endl;

				bool result = false;
				try {
					// Call the actual function
					result = func(args...);
				}
				catch (const std::exception& e) {
					std::cerr << "Exception in task wrapper: " << e.what() << std::endl;
				}

				// Clear thread tracking info
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					hasActiveTask = false;
					activeThreadId = std::thread::id{};
					terminateFlag = false;
				}

				std::cout << "Task completed for entity " << entityID << " with result: " << (result ? "success" : "failure") << std::endl;

				return result;
				};
		}

		// Static task functions - these match the actual function signatures
		static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
			try {
				// Ensure model is not still loading
				if (Utils::SDContextManager::IsContextLoading(metadata)) {
					std::cerr << "ERROR: Model is still loading, cannot start inference" << std::endl;
					return false;
				}
				return Utils::Txt2Img::RunInference(metadata, fullPath, context);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during inference: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunConversion(const nlohmann::json& metadata) {
			try {
				return Utils::Conversion::ConvertToGGUF(metadata);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during conversion: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
			try {
				// Ensure model is not still loading
				if (Utils::SDContextManager::IsContextLoading(metadata)) {
					std::cerr << "ERROR: Model is still loading, cannot start img2img" << std::endl;
					return false;
				}

				// Clone metadata and modify VAE settings for img2vid tasks
				nlohmann::json modifiedMetadata = metadata;

				// Ensure vae_decode_only is false for img2vid (needs full VAE)
				if (modifiedMetadata.contains("components") && modifiedMetadata["components"].is_array()) {
					for (size_t i = 0; i < modifiedMetadata["components"].size(); ++i) {
						auto& comp = modifiedMetadata["components"][i];
						if (comp.contains("Vae")) {
							auto& vae = comp["Vae"];
							vae["vae_decode_only"] = false;
						}
					}
				}

				return Utils::Img2Img::RunImg2Img(modifiedMetadata, fullPath, context);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2img: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
			try {
				// Ensure model is not still loading
				if (Utils::SDContextManager::IsContextLoading(metadata)) {
					std::cerr << "ERROR: Model is still loading, cannot start img2vid" << std::endl;
					return false;
				}

				// Clone metadata and modify VAE settings for img2vid tasks
				nlohmann::json modifiedMetadata = metadata;

				// Ensure vae_decode_only is false for img2vid (needs full VAE)
				if (modifiedMetadata.contains("components") && modifiedMetadata["components"].is_array()) {
					for (size_t i = 0; i < modifiedMetadata["components"].size(); ++i) {
						auto& comp = modifiedMetadata["components"][i];
						if (comp.contains("Vae")) {
							auto& vae = comp["Vae"];
							vae["vae_decode_only"] = false;
						}
					}
				}

				return Utils::Img2Vid::RunImg2Vid(modifiedMetadata, fullPath, context);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2vid: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
			try {
				// Ensure model is not still loading
				if (Utils::SDContextManager::IsContextLoading(metadata)) {
					std::cerr << "ERROR: Model is still loading, cannot start edit" << std::endl;
					return false;
				}

				// Clone metadata and modify VAE settings for edit tasks
				nlohmann::json modifiedMetadata = metadata;

				// Ensure vae_decode_only is false for edit (needs full VAE)
				if (modifiedMetadata.contains("components") && modifiedMetadata["components"].is_array()) {
					for (size_t i = 0; i < modifiedMetadata["components"].size(); ++i) {
						auto& comp = modifiedMetadata["components"][i];
						if (comp.contains("Vae")) {
							auto& vae = comp["Vae"];
							vae["vae_decode_only"] = false;
						}
					}
				}

				return Utils::Edit::RunEdit(modifiedMetadata, fullPath, context);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during edit: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				// Upscaling doesn't need sd_ctx_t parameter
				return Utils::Upscaling::RunUpscaling(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during upscaling: " << e.what() << std::endl;
				return false;
			}
		}

		// Helper to determine if a task type produces video output
		bool IsVideoTask(TaskType taskType) const {
			return taskType == TaskType::Img2Vid || taskType == TaskType::Edit;
		}

		// Helper to get appropriate file extension based on task type
		std::string GetOutputExtension(TaskType taskType) const {
			switch (taskType) {
			case TaskType::Img2Vid:
			case TaskType::Edit:
				return ".mp4";
			default:
				return ".png";
			}
		}

		// Queue processing methods
		void ProcessQueues() {
			std::lock_guard<std::mutex> lock(queueMutex);

			if (pauseWorker || shuttingDown) {
				return;
			}

			if (taskQueue.empty()) {
				return;
			}

			if (hasActiveTask) {
				return; // Don't start new tasks while one is running
			}

			auto& diffusionPool = Utils::ThreadPoolManager::getInstance().getDiffusionPool();

			// Find the first non-processing item that's ready
			for (auto& task : taskQueue) {
				if (!task.processing) {
					// Determine what context type this task needs (if not already determined)
					if (task.contextType == TaskData::NoContext) {
						task.contextType = GetRequiredContextType(task.taskType);
					}

					// Check if task is ready (has context if needed and not loading)
					if (!IsTaskReadyForProcessing(task)) {
						// Task not ready yet, try to acquire context if we haven't already
						if (!task.contextAcquired && !task.modelLoading) {
							// First attempt to get context
							bool acquired = AcquireContextForTask(task);
							if (!acquired) {
								// Task is loading, wait for it
								break; // Don't check other tasks, wait for this one
							}
						}
						else if (task.modelLoading) {
							// Already loading, wait for it
							break; // Don't check other tasks, wait for this one
						}
						else {
							// Can't get context for some reason
							std::cerr << "Failed to acquire context for task " << task.entityID << std::endl;
							task.CleanupContext();
							// Remove failed task
							entitiesNeedingCleanup.push_back(task.entityID);
							auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
								[&task](const TaskData& t) { return t.entityID == task.entityID; });
							if (it != taskQueue.end()) {
								taskQueue.erase(it);
							}
							break; // Try next task in next iteration
						}
					}

					// Task is ready, prepare the output path if not already done
					if (task.fullPath.empty() && mgr.HasComponent<OutputImageComponent>(task.entityID)) {
						auto& output = mgr.GetComponent<OutputImageComponent>(task.entityID);

						// Modify filename extension based on task type
						std::string baseName = output.fileName;
						std::string extension = GetOutputExtension(task.taskType);

						// Remove existing extension if any
						size_t lastDot = baseName.find_last_of('.');
						if (lastDot != std::string::npos) {
							baseName = baseName.substr(0, lastDot);
						}

						std::string fullFileName = baseName + extension;

						// Extract directory
						std::string outputDir = output.filePath;
						if (!outputDir.empty() && std::filesystem::path(outputDir).has_extension()) {
							outputDir = std::filesystem::path(outputDir).parent_path().string();
						}

						// Fallback directories
						if (outputDir.empty()) {
							std::string outputFolder = Utils::FilePathService::GetPath("OutputFolder");
							if (!outputFolder.empty() && outputFolder[0] != '\0') {
								outputDir = outputFolder;
							}
							else {
								std::string defaultProject = Utils::FilePathService::GetPath("DefaultProject");
								if (!defaultProject.empty() && defaultProject[0] != '\0') {
									outputDir = defaultProject;
								}
								else {
									outputDir = Utils::FilePathService::GetExecutableDir();
								}
							}
						}

						// Create unique filename
						task.fullPath = Utils::PngMetadata::CreateUniqueFilename(
							fullFileName,  // filename with extension
							outputDir      // directory
						);
					}

					// Submit appropriate function based on task type
					try {
						switch (task.taskType) {
						case TaskType::Inference:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunInference, task.metadata, task.fullPath, task.sdContext)
							);
							break;

						case TaskType::Conversion:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunConversion, task.metadata)
							);
							break;

						case TaskType::Img2Img:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunImg2Img, task.metadata, task.fullPath, task.sdContext)
							);
							break;

						case TaskType::Img2Vid:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunImg2Vid, task.metadata, task.fullPath, task.sdContext)
							);
							break;

						case TaskType::Edit:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunEdit, task.metadata, task.fullPath, task.sdContext)
							);
							break;

						case TaskType::Upscaling:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunUpscaling, task.metadata, task.fullPath)
							);
							break;

						default:
							std::cerr << "Unknown task type: " << static_cast<int>(task.taskType) << std::endl;
							task.CleanupContext();
							continue;
						}

						// Mark as processing and exit loop (only one task at a time)
						task.processing = true;
						std::cout << "Started processing task for entity " << task.entityID << std::endl;
						break; // Only start one task
					}
					catch (const std::exception& e) {
						std::cerr << "Failed to submit task: " << e.what() << std::endl;
						task.CleanupContext();
						// Add to cleanup list for failed task
						entitiesNeedingCleanup.push_back(task.entityID);
						// Remove the failed task
						auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
							[&task](const TaskData& t) { return t.entityID == task.entityID; });
						if (it != taskQueue.end()) {
							taskQueue.erase(it);
						}
						break; // Try next task in next iteration
					}
				}
			}
		}

		void CheckTaskCompletion() {
			if (taskQueue.empty()) {
				return;
			}

			std::vector<std::tuple<std::string, TaskType, EntityID>> completedTasks;

			{
				std::unique_lock<std::mutex> lock(queueMutex);

				for (auto it = taskQueue.begin(); it != taskQueue.end();) {
					if (it->processing && it->result.valid()) {
						// Check if the future is ready without blocking
						auto status = it->result.wait_for(std::chrono::milliseconds(0));

						if (status == std::future_status::ready) {
							EntityID entityID = it->entityID;
							std::string fullPath = it->fullPath;
							TaskType taskType = it->taskType;
							bool success = false;

							try {
								success = it->result.get();
							}
							catch (const std::exception& e) {
								std::cerr << "Exception retrieving task result: " << e.what() << std::endl;
							}

							// Clean up context BEFORE removing task
							it->CleanupContext();

							// Process the completed task ONLY if not shutting down
							if (!shuttingDown && success) {
								completedTasks.emplace_back(fullPath, taskType, entityID);
							}
							else {
								std::cerr << "Task failed for entity " << entityID << std::endl;

								// Clean up any files that might have been created
								if (std::filesystem::exists(fullPath)) {
									try {
										std::filesystem::remove(fullPath);
									}
									catch (const std::exception& e) {
										std::cerr << "Failed to remove partial file: " << e.what() << std::endl;
									}
								}
							}

							// Remove the completed task
							it = taskQueue.erase(it);

							std::cout << "Task completed. Remaining queue size: " << taskQueue.size() << std::endl;
						}
						else {
							// Task still running
							++it;
						}
					}
					else {
						// Task not processing or no valid future
						++it;
					}
				}
			}

			// Process completed tasks
			if (!shuttingDown) {
				for (const auto& [fullPath, taskType, entityID] : completedTasks) {
					ProcessCompletedTask(fullPath, taskType, entityID);
				}
			}
		}

		void ProcessCompletedTask(const std::string& fullPath, TaskType taskType, EntityID entityID) {
			try {
				if (shuttingDown) {
					return;
				}

				if (!std::filesystem::exists(fullPath)) {
					std::cerr << "Output file not found: " << fullPath << std::endl;
					return;
				}

				// Handle video output using VideoSystem
				if (IsVideoTask(taskType)) {
					LoadVideoViaVideoSystem(fullPath);
				}
				else {
					// Create a NEW entity for the output image
					EntityID newImageEntity = mgr.AddNewEntity();
					mgr.AddComponent<ImageComponent>(newImageEntity);

					// Load the output image to the new entity using ImageSystem
					LoadImageViaImageSystem(newImageEntity, fullPath);

					std::cout << "Created new image entity " << newImageEntity
						<< " for output: " << fullPath << std::endl;
				}

				std::cout << "Successfully processed completed task: " << fullPath << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Error processing completed task: " << e.what() << std::endl;

				try {
					if (std::filesystem::exists(fullPath)) {
						std::filesystem::remove(fullPath);
					}
				}
				catch (...) {
					// Ignore file cleanup errors
				}
			}
		}
	};
} // namespace ECS