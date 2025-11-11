#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"

// Include component headers
#include "SDCPPComponents.h"
#include "Components.h"

// Include your processing utilities
#include "Txt2Img.hpp"
#include "Img2Img.hpp"
#include "Img2Vid.hpp"
#include "Edit.hpp"
#include "Upscaling.hpp"
#include "Conversion.hpp"
#include "PngMetadataUtils.hpp"

// Include ImageSystem and VideoSystem for direct loading
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"

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
				HandleClearRequest();
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

				// DEBUG LOGGING: Check what image data we have
				std::cout << "=== QUEUE TASK DEBUG ===" << std::endl;
				std::cout << "Original entity: " << entityID << std::endl;
				std::cout << "Cloned entity: " << clonedEntity << std::endl;

				if (mgr.HasComponent<InputImageComponent>(clonedEntity)) {
					auto& input = mgr.GetComponent<InputImageComponent>(clonedEntity);
					std::cout << "InputImageComponent: '" << input.filePath << "' " << input.width << "x" << input.height << std::endl;
					std::cout << "InputFilePath: '" << input.inputFilePath << "'" << std::endl;
				}

				if (mgr.HasComponent<ImageComponent>(clonedEntity)) {
					auto& img = mgr.GetComponent<ImageComponent>(clonedEntity);
					std::cout << "ImageComponent: '" << img.filePath << "' " << img.width << "x" << img.height << std::endl;
				}
				std::cout << "=== END DEBUG ===" << std::endl;

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

				// Copy ALL image components from original to cloned entity

				// Copy ImageComponent data
				if (mgr.HasComponent<ImageComponent>(originalEntity) &&
					mgr.HasComponent<ImageComponent>(clonedEntity)) {

					auto& originalImg = mgr.GetComponent<ImageComponent>(originalEntity);
					auto& clonedImg = mgr.GetComponent<ImageComponent>(clonedEntity);

					// Copy ALL image data
					clonedImg.filePath = originalImg.filePath;
					clonedImg.fileName = originalImg.fileName;
					clonedImg.width = originalImg.width;
					clonedImg.height = originalImg.height;
					clonedImg.channels = originalImg.channels;

					std::cout << "Copied ImageComponent data to cloned entity " << clonedEntity
						<< ": " << originalImg.filePath << " " << originalImg.width << "x" << originalImg.height << std::endl;
				}

				// Copy InputImageComponent data
				if (mgr.HasComponent<InputImageComponent>(originalEntity) &&
					mgr.HasComponent<InputImageComponent>(clonedEntity)) {

					auto& originalInput = mgr.GetComponent<InputImageComponent>(originalEntity);
					auto& clonedInput = mgr.GetComponent<InputImageComponent>(clonedEntity);

					// Copy file path information for ImageSystem
					clonedInput.filePath = originalInput.filePath;
					clonedInput.fileName = originalInput.fileName;
					clonedInput.width = originalInput.width;
					clonedInput.height = originalInput.height;
					clonedInput.channels = originalInput.channels;
					clonedInput.inputFilePath = originalInput.inputFilePath;

					std::cout << "Copied InputImageComponent data to cloned entity " << clonedEntity
						<< ": " << originalInput.filePath << " " << originalInput.width << "x" << originalInput.height << std::endl;
				}

				std::cout << "Successfully cloned entity " << originalEntity << " to " << clonedEntity << std::endl;
				return clonedEntity;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception cloning entity " << originalEntity << ": " << e.what() << std::endl;
				return 0;
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

		// Helper to load video via VideoSystem - creates NEW entity for output
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

				// Store the last generated video entity for VideoDiffusionView
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

		// The actual clearing logic - called from Update when it's safe
		void HandleClearRequest() {
			std::lock_guard<std::mutex> lock(queueMutex);
			std::cout << "Clearing queue with " << taskQueue.size() << " items" << std::endl;

			// Just remove all non-processing tasks - NO CLEANUP
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

		// Static task functions - FIXED Img2Img to handle missing InputImage data
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

						// Use filePath as directory - extract directory if it's a file path
						std::string outputDir = output.filePath;

						// If filePath contains a filename, extract just the directory
						if (!outputDir.empty()) {
							std::filesystem::path p(outputDir);
							if (p.has_extension()) {
								outputDir = p.parent_path().string();
							}
						}

						// Fallback to default if empty
						if (outputDir.empty()) {
							outputDir = Utils::FilePaths::outputFolderPath.empty()
								? Utils::FilePaths::defaultProjectPath
								: Utils::FilePaths::outputFolderPath;
						}

						// Use proper API signature for CreateUniqueFilename
						task.fullPath = Utils::PngMetadata::CreateUniqueFilename(
							fullFileName,  // filename with extension
							outputDir      // directory
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

			std::vector<std::tuple<std::string, TaskType, EntityID>> completedTasks;

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

							// Only clean up cloned entities
							if (it->isClonedEntity) {
								entitiesNeedingCleanup.push_back(entityID);
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
			} // Lock released here

			// Process completed tasks WITHOUT holding the lock
			if (!shuttingDown) {
				for (const auto&[fullPath, taskType, originalEntity] : completedTasks) {
					ProcessCompletedTask(fullPath, taskType, originalEntity);
				}
			}
		}

		void ProcessCompletedTask(const std::string& fullPath, TaskType taskType, EntityID clonedEntity) {
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