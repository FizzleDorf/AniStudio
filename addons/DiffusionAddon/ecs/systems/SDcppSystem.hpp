#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"

// CRITICAL: Include the asset management system
#include "AssetManager.hpp"
#include "AssetHandleComponents.hpp"

// Include component headers properly
#include "components.h"

// Include your processing utilities
#include "Txt2Img.hpp"
#include "Img2Img.hpp"
#include "Img2Vid.hpp"
#include "Edit.hpp"
#include "Upscaling.hpp"
#include "Conversion.hpp"
#include "PngMetadataUtils.hpp"

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

		// Internal tracking structure with futures and metadata
		struct TaskData {
			EntityID entityID = 0;
			bool processing = false;
			TaskType taskType;
			nlohmann::json metadata;
			std::string fullPath;
			std::future<bool> result;
			bool isClonedEntity = false;

			// Default constructor
			TaskData() = default;

			// Move constructor
			TaskData(TaskData&& other) noexcept
				: entityID(other.entityID)
				, processing(other.processing)
				, taskType(other.taskType)
				, metadata(std::move(other.metadata))
				, fullPath(std::move(other.fullPath))
				, result(std::move(other.result))
				, isClonedEntity(other.isClonedEntity) {}

			// Move assignment operator
			TaskData& operator=(TaskData&& other) noexcept {
				if (this != &other) {
					entityID = other.entityID;
					processing = other.processing;
					taskType = other.taskType;
					metadata = std::move(other.metadata);
					fullPath = std::move(other.fullPath);
					result = std::move(other.result);
					isClonedEntity = other.isClonedEntity;
				}
				return *this;
			}

			// Delete copy operations since std::future is not copyable
			TaskData(const TaskData&) = delete;
			TaskData& operator=(const TaskData&) = delete;
		};

		// Constructor - Use single thread for diffusion tasks
		SDCPPSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr)
			, pauseWorker(false)
			, hasActiveTask(false)
			, clearRequested(false) {
			sysName = "SDCPPSystem";

			// Use proper component signatures for the new asset system
			AddComponentSignature<LatentComponent>();
			AddComponentSignature<ImageHandleComponent>();
			AddComponentSignature<VideoHandleComponent>();

			// Add these components - they should exist based on your component files
			AddComponentSignature<OutputImageComponent>();
			AddComponentSignature<InputImageComponent>();
		}

		// Destructor
		~SDCPPSystem() {
			// Signal shutdown and wait for all tasks to complete
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				shuttingDown = true;
				pauseWorker = true;
			}

			// Try to terminate the active task if it exists
			TerminateActiveTask();

			// Get the diffusion pool and wait for tasks to complete
			auto& diffusionPool = Utils::ThreadPoolManager::getInstance().getDiffusionPool();

			// Wait for diffusion tasks to complete (with timeout)
			auto future = std::async(std::launch::async, [&diffusionPool]() {
				diffusionPool.waitForTasks();
			});

			if (future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
				std::cerr << "Warning: Diffusion pool did not shut down cleanly within timeout" << std::endl;
			}

			// Clean up any remaining entities
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				HandleClearRequestInternal();
			}
		}

		// Public methods
		void QueueTask(const EntityID entityID, const TaskType taskType) {
			// Validate entity exists first
			if (!mgr.GetEntitiesSignatures().count(entityID)) {
				std::cerr << "Error: Entity " << entityID << " does not exist!" << std::endl;
				return;
			}

			try {
				std::lock_guard<std::mutex> lock(queueMutex);

				// Don't accept new tasks during shutdown
				if (shuttingDown) {
					std::cerr << "Cannot queue task during shutdown" << std::endl;
					return;
				}

				// Create a cloned entity for processing to preserve original
				EntityID clonedEntity = CloneEntityForProcessing(entityID);
				if (clonedEntity == 0) {
					std::cerr << "Failed to clone entity " << entityID << " for processing" << std::endl;
					return;
				}

				// Create task data
				TaskData taskData;
				taskData.entityID = clonedEntity;  // Use cloned entity
				taskData.processing = false;
				taskData.taskType = taskType;
				taskData.isClonedEntity = true;

				// Check if we need to generate a random seed for certain task types
				if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
					taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {

					if (mgr.HasComponent<SamplerComponent>(clonedEntity)) {
						auto& samplerComp = mgr.GetComponent<SamplerComponent>(clonedEntity);

						// Generate random seed if needed
						if (samplerComp.seed < 0) {
							uint64_t newSeed = Utils::generateRandomSeed();
							samplerComp.seed = static_cast<int>(newSeed);
							std::cout << "Generated random seed: " << samplerComp.seed << std::endl;
						}
					}
				}

				// Serialize entity to metadata
				taskData.metadata = mgr.SerializeEntity(clonedEntity);
				std::cout << "Successfully serialized cloned entity " << clonedEntity << " (original: " << entityID << ")" << std::endl;

				// Add to internal task list
				taskQueue.push_back(std::move(taskData));

				std::cout << "Entity " << clonedEntity << " (cloned from " << entityID << ") queued for processing. Queue position: " << taskQueue.size() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in QueueTask: " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in QueueTask!" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			if (shuttingDown) {
				return;
			}

			// Handle clear request at the beginning of update cycle
			if (clearRequested) {
				HandleClearRequest();
				clearRequested = false;
			}

			// Process queues - start next task if no task is currently running
			ProcessQueues();

			// Update status of running task
			CheckTaskCompletion();

			// Handle deferred entity cleanup
			HandleDeferredCleanup();
		}

		void RemoveFromQueue(const size_t index) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (index < taskQueue.size() && !taskQueue[index].processing) {
				EntityID entityID = taskQueue[index].entityID;
				taskQueue.erase(taskQueue.begin() + index);

				// Add to deferred cleanup list instead of destroying immediately
				entitiesNeedingCleanup.push_back(entityID);
			}
		}

		void MoveInQueue(const size_t fromIndex, const size_t toIndex) {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size())
				return;
			if (taskQueue[fromIndex].processing)
				return;

			// Use move semantics for efficiency
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
				// TODO: needs logic in new sdcpp implementation
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
			std::lock_guard<std::mutex> lock(queueMutex);
			pauseWorker = false;
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

	private:
		// Private member variables
		std::vector<TaskData> taskQueue;
		std::atomic<bool> pauseWorker;
		std::atomic<bool> shuttingDown{ false };
		std::atomic<bool> terminateFlag{ false };
		std::atomic<bool> clearRequested{ false };
		std::vector<EntityID> entitiesNeedingCleanup;
		mutable std::mutex queueMutex;
		EntityID lastGeneratedVideoEntity{ 0 }; // Track last generated video

		Utils::ThreadPoolManager::PoolStats GetThreadPoolStats() const {
			return Utils::ThreadPoolManager::getInstance().getStats();
		}

		// Single task tracking
		bool hasActiveTask{ false };
		std::thread::id activeThreadId{};

		// Clone entity for processing to preserve original input images
		EntityID CloneEntityForProcessing(const EntityID originalEntity) {
			try {
				// Serialize the original entity
				nlohmann::json entityData = mgr.SerializeEntity(originalEntity);

				// Create a new entity from the serialized data
				EntityID clonedEntity = mgr.DeserializeEntity(entityData);

				if (clonedEntity == 0) {
					std::cerr << "Failed to deserialize cloned entity" << std::endl;
					return 0;
				}

				// Special handling for InputImageComponent with AssetManager integration
				if (mgr.HasComponent<InputImageComponent>(originalEntity) &&
					mgr.HasComponent<InputImageComponent>(clonedEntity)) {

					auto& originalInput = mgr.GetComponent<InputImageComponent>(originalEntity);
					auto& clonedInput = mgr.GetComponent<InputImageComponent>(clonedEntity);

					// With the new AssetManager system, we copy the asset references instead of raw data
					if (originalInput.imageAssetId != INVALID_RESOURCE_ID) {
						clonedInput.imageAssetId = originalInput.imageAssetId;
						clonedInput.textureAssetId = originalInput.textureAssetId;

						// Copy the cached properties
						clonedInput.width = originalInput.width;
						clonedInput.height = originalInput.height;
						clonedInput.channels = originalInput.channels;
						clonedInput.fileName = originalInput.fileName;
						clonedInput.filePath = originalInput.filePath;
						clonedInput.textureID = originalInput.textureID;

						std::cout << "Copied asset references for cloned entity " << clonedEntity << std::endl;
					}
					else {
						// Fallback: If no asset is loaded, we can't clone image data
						// The processing will need to handle this case
						std::cout << "Warning: No image asset to clone for entity " << clonedEntity << std::endl;
					}
				}

				// Handle other image component types similarly
				if (mgr.HasComponent<ImageComponent>(originalEntity) &&
					mgr.HasComponent<ImageComponent>(clonedEntity)) {

					auto& originalImg = mgr.GetComponent<ImageComponent>(originalEntity);
					auto& clonedImg = mgr.GetComponent<ImageComponent>(clonedEntity);

					// Copy asset references
					if (originalImg.imageAssetId != INVALID_RESOURCE_ID) {
						clonedImg.imageAssetId = originalImg.imageAssetId;
						clonedImg.textureAssetId = originalImg.textureAssetId;

						// Copy cached properties
						clonedImg.width = originalImg.width;
						clonedImg.height = originalImg.height;
						clonedImg.channels = originalImg.channels;
						clonedImg.fileName = originalImg.fileName;
						clonedImg.filePath = originalImg.filePath;
						clonedImg.textureID = originalImg.textureID;
					}
				}

				std::cout << "Successfully cloned entity " << originalEntity << " to " << clonedEntity << std::endl;
				return clonedEntity;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception cloning entity " << originalEntity << ": " << e.what() << std::endl;
				return 0;
			}
		}
		// The actual clearing logic - called from Update when it's safe
		void HandleClearRequest() {
			std::lock_guard<std::mutex> lock(queueMutex);
			HandleClearRequestInternal();
		}

		void HandleClearRequestInternal() {
			std::cout << "Clearing queue with " << taskQueue.size() << " items" << std::endl;

			// Collect entities from non-processing tasks
			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				if (!it->processing) {
					EntityID entityID = it->entityID;
					entitiesNeedingCleanup.push_back(entityID);
					it = taskQueue.erase(it);
					std::cout << "Queued entity " << entityID << " for cleanup" << std::endl;
				}
				else {
					std::cout << "Keeping processing task for entity " << it->entityID << std::endl;
					++it;
				}
			}

			std::cout << "Queue cleared. " << taskQueue.size() << " items remaining (processing)" << std::endl;
		}

		// Handle deferred entity cleanup
		void HandleDeferredCleanup() {
			if (entitiesNeedingCleanup.empty()) {
				return;
			}

			// Process a few entities per frame to avoid hitches
			int maxCleanupPerFrame = 5;
			int cleaned = 0;

			for (auto it = entitiesNeedingCleanup.begin();
				it != entitiesNeedingCleanup.end() && cleaned < maxCleanupPerFrame;) {

				EntityID entityID = *it;

				try {
					if (mgr.GetEntitiesSignatures().count(entityID)) {
						// Always destroy cloned entities completely - they are temporary
						mgr.DestroyEntity(entityID);
						std::cout << "Cleaned up cloned entity " << entityID << std::endl;
					}
					it = entitiesNeedingCleanup.erase(it);
					cleaned++;
				}
				catch (const std::exception& e) {
					std::cerr << "Error in deferred cleanup: " << e.what() << std::endl;
					it = entitiesNeedingCleanup.erase(it);
					cleaned++;
				}
			}

			if (cleaned > 0) {
				std::cout << "Deferred cleanup: processed " << cleaned << " entities, "
					<< entitiesNeedingCleanup.size() << " remaining" << std::endl;
			}
		}

		void TerminateActiveTask() {
			std::lock_guard<std::mutex> lock(queueMutex);
			if (hasActiveTask) {
				terminateFlag = true;
				std::cout << "Termination flag set for active task" << std::endl;
			}
		}

		// Task wrapper that captures thread ID and sets active task status
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

		// Static task functions
		static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::Txt2Img::RunInference(metadata, fullPath);
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

		static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::Img2Img::RunImg2Img(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2img: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::Img2Vid::RunImg2Vid(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2vid: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::Edit::RunEdit(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during edit: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
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
				return;
			}

			auto& diffusionPool = Utils::ThreadPoolManager::getInstance().getDiffusionPool();

			// Find the first non-processing item
			for (auto& task : taskQueue) {
				if (!task.processing) {
					// Prepare the output path based on task type
					if (mgr.HasComponent<OutputImageComponent>(task.entityID)) {
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

						// Use proper API signature for CreateUniqueFilename
						task.fullPath = Utils::PngMetadata::CreateUniqueFilename(
							output.outputDirectory,  // directory
							fullFileName             // filename with extension
						);
					}

					// Submit appropriate function based on task type using wrapper
					try {
						switch (task.taskType) {
						case TaskType::Inference:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunInference, task.metadata, task.fullPath)
							);
							break;

						case TaskType::Conversion:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunConversion, task.metadata)
							);
							break;

						case TaskType::Img2Img:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunImg2Img, task.metadata, task.fullPath)
							);
							break;

						case TaskType::Img2Vid:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunImg2Vid, task.metadata, task.fullPath)
							);
							break;

						case TaskType::Edit:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunEdit, task.metadata, task.fullPath)
							);
							break;

						case TaskType::Upscaling:
							task.result = diffusionPool.submit(
								CreateTaskWrapper(task.entityID, RunUpscaling, task.metadata, task.fullPath)
							);
							break;

						default:
							std::cerr << "Unknown task type: " << static_cast<int>(task.taskType) << std::endl;
							continue;
						}

						// Mark as processing and exit loop (only one task at a time)
						task.processing = true;
						std::cout << "Started processing task for entity " << task.entityID << std::endl;
						break;
					}
					catch (const std::exception& e) {
						std::cerr << "Failed to submit task: " << e.what() << std::endl;
						// Add to cleanup list for failed task
						entitiesNeedingCleanup.push_back(task.entityID);
						// Remove the failed task
						auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
							[&task](const TaskData& t) { return t.entityID == task.entityID; });
						if (it != taskQueue.end()) {
							taskQueue.erase(it);
						}
						break;
					}
				}
			}
		}

		void CheckTaskCompletion() {
			if (taskQueue.empty()) {
				return;
			}

			std::vector<std::tuple<EntityID, std::string, TaskType>> completedTasks;

			{
				std::unique_lock<std::mutex> lock(queueMutex);

				for (auto it = taskQueue.begin(); it != taskQueue.end();) {
					// Check if this task is processing and has a valid future
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

							// Process the completed task
							if (success) {
								completedTasks.emplace_back(entityID, fullPath, taskType);
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
								// Add failed entity to cleanup list
								entitiesNeedingCleanup.push_back(entityID);
							}

							// Remove the completed task - this will allow the next task to start
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
			} // Lock released here

			// Process completed tasks WITHOUT holding the lock
			for (const auto&[entityID, fullPath, taskType] : completedTasks) {
				ProcessCompletedTask(entityID, fullPath, taskType);
			}
		}

		void ProcessCompletedTask(const EntityID entityID, const std::string& fullPath, TaskType taskType) {
			try {
				if (shuttingDown) {
					return;
				}

				if (!mgr.GetEntitiesSignatures().count(entityID)) {
					std::cerr << "Entity no longer exists: " << entityID << std::endl;
					return;
				}

				if (!std::filesystem::exists(fullPath)) {
					std::cerr << "Output file not found: " << fullPath << std::endl;
					return;
				}

				// Handle video output using AssetManager
				if (IsVideoTask(taskType)) {
					// Create a new entity for the generated video
					EntityID videoEntity = mgr.AddNewEntity();

					// Add the new asset handle component
					auto& videoHandle = mgr.AddComponent<VideoHandleComponent>(videoEntity);

					// Also add the old VideoComponent for backward compatibility
					mgr.AddComponent<VideoComponent>(videoEntity);

					// Use AssetManager to load the video asynchronously
					auto future = AssetManager::Instance().LoadVideoAsync(fullPath,
						[this, videoEntity](ResourceID assetId, bool success) {
						if (success) {
							std::cout << "[SDCPPSystem] Video loaded successfully (Entity: "
								<< videoEntity << ", AssetID: " << assetId << ")" << std::endl;

							// Update the handle component with the asset ID
							if (mgr.HasComponent<VideoHandleComponent>(videoEntity)) {
								VideoHandleComponent& handle = mgr.GetComponent<VideoHandleComponent>(videoEntity);
								handle.videoAssetId = assetId;
							}
						}
						else {
							std::cerr << "[SDCPPSystem] Failed to load video for entity " << videoEntity << std::endl;
						}
					});

					// Store the last generated video entity for VideoDiffusionView
					{
						std::lock_guard<std::mutex> lock(queueMutex);
						lastGeneratedVideoEntity = videoEntity;
					}

					std::cout << "Video loading started for entity " << videoEntity << " from " << fullPath << std::endl;
				}
				else {
					// Handle image output using AssetManager

					// Ensure entity has image handle component
					if (!mgr.HasComponent<ImageHandleComponent>(entityID)) {
						mgr.AddComponent<ImageHandleComponent>(entityID);
					}

					// Also add the old ImageComponent for backward compatibility
					if (!mgr.HasComponent<ImageComponent>(entityID)) {
						mgr.AddComponent<ImageComponent>(entityID);
					}

					// Use AssetManager to load the image asynchronously
					auto future = AssetManager::Instance().LoadImageAsync(fullPath,
						[this, entityID](ResourceID assetId, bool success) {
						if (success) {
							std::cout << "[SDCPPSystem] Image loaded successfully (Entity: "
								<< entityID << ", AssetID: " << assetId << ")" << std::endl;

							// Update the handle component with the asset ID
							if (mgr.HasComponent<ImageHandleComponent>(entityID)) {
								ImageHandleComponent& handle = mgr.GetComponent<ImageHandleComponent>(entityID);
								handle.imageAssetId = assetId;

								// Auto-create texture if needed
								if (handle.autoCreateTexture) {
									handle.textureAssetId = AssetManager::Instance().CreateTextureFromImage(assetId);
								}
							}
						}
						else {
							std::cerr << "[SDCPPSystem] Failed to load image for entity " << entityID << std::endl;
						}
					});

					std::cout << "Image loading started for entity " << entityID << " from " << fullPath << std::endl;
				}

				std::cout << "Successfully processed completed task for entity " << entityID << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Error processing completed task: " << e.what() << std::endl;
				entitiesNeedingCleanup.push_back(entityID);

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