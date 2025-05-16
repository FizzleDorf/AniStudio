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

				// Lock and add to queue
				std::lock_guard<std::mutex> lock(queueMutex);
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
			for (auto& item : taskQueue) {
				if (item.processing && item.task) {
					item.task->cancel();
				}
			}
		}

		void ClearQueue() {
			std::lock_guard<std::mutex> lock(queueMutex);

			// Remove all non-processing tasks and destroy their entities
			for (auto it = taskQueue.begin(); it != taskQueue.end();) {
				if (!it->processing) {
					mgr.DestroyEntity(it->entityID);
					it = taskQueue.erase(it);
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
		std::shared_ptr<Utils::Task> CreateInferenceTask(const EntityID entityID, const nlohmann::json& metadata) {
			return std::make_shared<InferenceTask>(metadata);
		}

		std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID, const nlohmann::json& metadata) {
			return std::make_shared<ConvertTask>(metadata);
		}

		std::shared_ptr<Utils::Task> CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata) {
			return std::make_shared<Img2ImgTask>(metadata);
		}

		std::shared_ptr<Utils::Task> CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata) {
			return std::make_shared<UpscalingTask>(metadata);
		}

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
			try {
				if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
					throw std::runtime_error("Missing OutputImageComponent");
				}

				if (!mgr.GetEntitiesSignatures().count(entityID)) {
					throw std::runtime_error("Entity no longer exists");
				}

				auto& outputComp = mgr.GetComponent<OutputImageComponent>(entityID);

				if (outputComp.filePath.empty()) {
					throw std::runtime_error("No filepath in OutputImageComponent");
				}

				auto imageSystem = mgr.GetSystem<ImageSystem>();
				if (!imageSystem) {
					throw std::runtime_error("ImageSystem not found");
				}

				// Create/update Image component
				if (!mgr.HasComponent<ImageComponent>(entityID)) {
					auto& imageComp = mgr.AddComponent<ImageComponent>(entityID);
					imageComp.filePath = outputComp.filePath;
					imageComp.fileName = std::filesystem::path(outputComp.filePath).filename().string();
				}
				else {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entityID);
					imageComp.filePath = outputComp.filePath;
					imageComp.fileName = std::filesystem::path(outputComp.filePath).filename().string();
				}

				// Load the image data through the ImageSystem
				imageSystem->SetImage(entityID, outputComp.filePath);

				if (!std::filesystem::exists(outputComp.filePath)) {
					throw std::runtime_error("Failed to find output image file!");
				}

				// Clean up unnecessary components, leaving only the ImageComponent
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
				mgr.DestroyEntity(entityID);
			}
		}
	};

} // namespace ECS