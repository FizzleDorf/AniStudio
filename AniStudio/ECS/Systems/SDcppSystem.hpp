/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "ImageUtils.hpp"
#include "ImageSystem.hpp"
#include "SDCPPComponents.h"
#include "SDcppUtils.hpp"
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
#include <optional>
#include <future>
#include <thread>

namespace ECS {
	class SDCPPSystem : public BaseSystem {
	public:
		enum class TaskType {
			Inference,
			Conversion,
			Img2Img,
			Upscaling
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

			// Default constructor
			TaskData() = default;

			// Move constructor
			TaskData(TaskData&& other) noexcept
				: entityID(other.entityID)
				, processing(other.processing)
				, taskType(other.taskType)
				, metadata(std::move(other.metadata))
				, fullPath(std::move(other.fullPath))
				, result(std::move(other.result)) {}

			// Move assignment operator
			TaskData& operator=(TaskData&& other) noexcept {
				if (this != &other) {
					entityID = other.entityID;
					processing = other.processing;
					taskType = other.taskType;
					metadata = std::move(other.metadata);
					fullPath = std::move(other.fullPath);
					result = std::move(other.result);
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
			, hasActiveTask(false) {
			sysName = "SDCPPSystem";
			AddComponentSignature<LatentComponent>();
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
				ClearQueueInternal();
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

				// Create task data
				TaskData taskData;
				taskData.entityID = entityID;
				taskData.processing = false;
				taskData.taskType = taskType;

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
				taskData.metadata = mgr.SerializeEntity(entityID);
				std::cout << "Successfully serialized entity " << entityID << std::endl;

				// Add to internal task list
				taskQueue.push_back(std::move(taskData));

				std::cout << "Entity " << entityID << " queued for processing. Queue position: " << taskQueue.size() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in QueueTask: " << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << "Unknown exception in QueueTask!" << std::endl;
			}
		}

		void Update(const float deltaT) override {
			// Don't process if shutting down
			if (shuttingDown) {
				return;
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

				// Clean up the entity
				try {
					if (mgr.GetEntitiesSignatures().count(entityID)) {
						mgr.DestroyEntity(entityID);
					}
				}
				catch (const std::exception& e) {
					std::cerr << "Error destroying entity in RemoveFromQueue: " << e.what() << std::endl;
				}
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
			std::lock_guard<std::mutex> lock(queueMutex);
			ClearQueueInternal();
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

		// New methods for single-threaded processing
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

	private:
		// Private member variables
		std::vector<TaskData> taskQueue;
		std::atomic<bool> pauseWorker;
		std::atomic<bool> shuttingDown{ false };
		std::atomic<bool> terminateFlag{ false };
		mutable std::mutex queueMutex;

		Utils::ThreadPoolManager::PoolStats GetThreadPoolStats() const {
			return Utils::ThreadPoolManager::getInstance().getStats();
		}

		// Single task tracking
		bool hasActiveTask{ false };
		std::thread::id activeThreadId{};

		// Internal queue clearing (must be called with lock held)
		void ClearQueueInternal() {
			if (taskQueue.empty()) {
				return;
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
						std::cerr << "Error in ClearQueueInternal: " << e.what() << std::endl;
					}
				}
				else {
					++it;
				}
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
				return Utils::SDCPPUtils::RunInference(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during inference: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunConversion(const nlohmann::json& metadata) {
			try {
				return Utils::SDCPPUtils::ConvertToGGUF(metadata);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during conversion: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::SDCPPUtils::RunImg2Img(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2img: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath) {
			try {
				return Utils::SDCPPUtils::RunUpscaling(metadata, fullPath);
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during upscaling: " << e.what() << std::endl;
				return false;
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

			// Only start a new task if no task is currently active
			if (hasActiveTask) {
				return;
			}

			// Get the diffusion thread pool
			auto& diffusionPool = Utils::ThreadPoolManager::getInstance().getDiffusionPool();

			// Find the first non-processing item
			for (auto& task : taskQueue) {
				if (!task.processing) {
					// Prepare the output path
					if (mgr.HasComponent<OutputImageComponent>(task.entityID)) {
						auto& output = mgr.GetComponent<OutputImageComponent>(task.entityID);
						task.fullPath = Utils::PngMetadata::CreateUniqueFilename(output.fileName, output.filePath);
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
						// Remove the failed task
						auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
							[&task](const TaskData& t) { return t.entityID == task.entityID; });
						if (it != taskQueue.end()) {
							EntityID entityID = it->entityID;
							taskQueue.erase(it);
							if (mgr.GetEntitiesSignatures().count(entityID)) {
								mgr.DestroyEntity(entityID);
							}
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

			std::unique_lock<std::mutex> lock(queueMutex);

			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				// Check if this task is processing and has a valid future
				if (it->processing && it->result.valid()) {
					// Check if the future is ready without blocking
					auto status = it->result.wait_for(std::chrono::milliseconds(0));

					if (status == std::future_status::ready) {
						// Store the entity ID and path before erasing
						EntityID entityID = it->entityID;
						std::string fullPath = it->fullPath;
						bool success = false;

						try {
							// Get the result
							success = it->result.get();
						}
						catch (const std::exception& e) {
							std::cerr << "Exception retrieving task result: " << e.what() << std::endl;
						}

						// Process the completed task
						if (success) {
							// Must unlock since ProcessCompletedTask may acquire locks
							lock.unlock();
							ProcessCompletedTask(entityID, fullPath);
							lock.lock();
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
		}

		void ProcessCompletedTask(const EntityID entityID, const std::string& fullPath) {
			try {
				if (!mgr.GetEntitiesSignatures().count(entityID)) {
					std::cerr << "Entity no longer exists: " << entityID << std::endl;
					return;
				}

				// Process the completed task
				auto imageSystem = mgr.GetSystem<ImageSystem>();
				if (!imageSystem) {
					std::cerr << "ImageSystem not found" << std::endl;
					return;
				}

				// Create/update Image component if needed
				if (!mgr.HasComponent<ImageComponent>(entityID)) {
					mgr.AddComponent<ImageComponent>(entityID);
				}

				// Use ImageSystem to load the generated image
				if (std::filesystem::exists(fullPath)) {
					imageSystem->SetImage(entityID, fullPath);
					std::cout << "Image loaded successfully for entity " << entityID << std::endl;
				}
				else {
					std::cerr << "Failed to find output image file: " << fullPath << std::endl;
					return;
				}

				// Clean up unnecessary components, keeping only Image component
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

				// Try to clean up on error
				try {
					auto imageSystem = mgr.GetSystem<ImageSystem>();
					if (imageSystem && mgr.HasComponent<ImageComponent>(entityID)) {
						imageSystem->RemoveImage(entityID);
					}
					else if (mgr.GetEntitiesSignatures().count(entityID)) {
						mgr.DestroyEntity(entityID);
					}
				}
				catch (const std::exception& e2) {
					std::cerr << "Error destroying entity: " << e2.what() << std::endl;
				}
			}
		}
	};
} // namespace ECS