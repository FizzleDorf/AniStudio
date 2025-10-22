#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ThreadPool.hpp"
#include "ImageUtils.hpp"
#include <memory>
#include <functional>
#include <stb_image.h>
#include <stb_image_write.h>
#include <queue>
#include <mutex>
#include <future>

namespace ECS {

	class ImageSystem : public BaseSystem {
	public:
		using ImageCallback = std::function<void(EntityID)>;

		struct LoadResult {
			bool success = false;
			unsigned char* data = nullptr;
			int width = 0;
			int height = 0;
			int channels = 0;
			std::string fileName;
			std::string filePath;
			EntityID entityID = 0;
		};

		struct LoadingTask {
			EntityID entityID;
			std::string filePath;
			std::future<LoadResult> future;

			LoadingTask() = default;

			LoadingTask(LoadingTask&& other) noexcept
				: entityID(other.entityID)
				, filePath(std::move(other.filePath))
				, future(std::move(other.future)) {}

			LoadingTask& operator=(LoadingTask&& other) noexcept {
				if (this != &other) {
					entityID = other.entityID;
					filePath = std::move(other.filePath);
					future = std::move(other.future);
				}
				return *this;
			}

			LoadingTask(const LoadingTask&) = delete;
			LoadingTask& operator=(const LoadingTask&) = delete;
		};

		ImageSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr) {
			sysName = "ImageSystem";
			AddComponentSignature<ImageComponent>();
		}

		~ImageSystem() override {
			std::cout << "[ImageSystem] Destructor - cleaning up" << std::endl;
			// No texture cleanup here - TextureSystem handles that
		}

		void Start() override {
			std::cout << "[ImageSystem] Started" << std::endl;
			// Load images for existing entities with ImageComponent
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
					if (!imageComp.filePath.empty()) {
						LoadImageAsync(entity, imageComp.filePath);
					}
				}
			}
		}

		void Update(const float deltaT) override {
			ProcessCompletedLoads();
		}

		void RegisterImageAddedCallback(const ImageCallback& callback) {
			imageAddedCallbacks.push_back(callback);
		}

		void RegisterImageRemovedCallback(const ImageCallback& callback) {
			imageRemovedCallbacks.push_back(callback);
		}

		void SetImage(const EntityID entity, const std::string& filePath) {
			if (mgr.HasComponent<ImageComponent>(entity)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

				// Clear input image data if this is an InputImageComponent
				if (mgr.HasComponent<InputImageComponent>(entity)) {
					auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
					inputComp.ClearImageData();
				}

				// Start async loading
				LoadImageAsync(entity, filePath);
			}
		}

		void RemoveImage(const EntityID entity) {
			if (mgr.HasComponent<ImageComponent>(entity)) {
				// Cancel any pending load
				CancelPendingLoad(entity);

				// Clear input image data if this is an InputImageComponent
				if (mgr.HasComponent<InputImageComponent>(entity)) {
					auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
					inputComp.ClearImageData();
				}

				// Notify callbacks BEFORE destroying entity
				// TextureSystem will handle texture cleanup via callback
				NotifyImageRemoved(entity);

				// Destroy the entity
				mgr.DestroyEntity(entity);
			}
		}

		std::vector<EntityID> GetAllImageEntities() const {
			std::vector<EntityID> result;
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					result.push_back(entity);
				}
			}
			return result;
		}

	private:
		std::vector<ImageCallback> imageAddedCallbacks;
		std::vector<ImageCallback> imageRemovedCallbacks;
		std::vector<LoadingTask> pendingLoads;
		std::mutex loadMutex;

		void LoadImageAsync(EntityID entity, const std::string& filePath) {
			auto& ioPool = Utils::ThreadPoolManager::getInstance().getIOPool();

			std::cout << "[ImageSystem] Starting async load for entity " << entity << ": " << filePath << std::endl;

			// Create async task
			auto future = ioPool.submit([filePath, entity]() -> LoadResult {
				LoadResult result;
				result.filePath = filePath;
				result.entityID = entity;

				// Extract filename
				size_t lastSlash = filePath.find_last_of("/\\");
				if (lastSlash != std::string::npos) {
					result.fileName = filePath.substr(lastSlash + 1);
				}
				else {
					result.fileName = filePath;
				}

				std::cout << "[ImageSystem] I/O Thread: Loading " << result.fileName << std::endl;

				// Load image data on I/O thread
				result.data = Utils::ImageUtils::LoadImageData(filePath, result.width, result.height, result.channels);
				result.success = (result.data != nullptr);

				if (result.success) {
					std::cout << "[ImageSystem] I/O Thread: Successfully loaded " << result.fileName
						<< " (" << result.width << "x" << result.height << ")" << std::endl;
				}
				else {
					std::cerr << "[ImageSystem] I/O Thread: Failed to load " << result.fileName << std::endl;
				}

				return result;
			});

			// Store the task
			std::lock_guard<std::mutex> lock(loadMutex);
			LoadingTask task;
			task.entityID = entity;
			task.filePath = filePath;
			task.future = std::move(future);
			pendingLoads.push_back(std::move(task));
		}

		void ProcessCompletedLoads() {
			std::lock_guard<std::mutex> lock(loadMutex);

			for (auto it = pendingLoads.begin(); it != pendingLoads.end();) {
				if (it->future.valid() &&
					it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {

					try {
						LoadResult result = it->future.get();

						std::cout << "[ImageSystem] Processing completed load for entity "
							<< result.entityID << std::endl;

						// Check if entity still exists
						if (mgr.HasComponent<ImageComponent>(result.entityID)) {
							auto& imageComp = mgr.GetComponent<ImageComponent>(result.entityID);

							if (result.success) {
								// Update base ImageComponent properties
								imageComp.width = result.width;
								imageComp.height = result.height;
								imageComp.channels = result.channels;
								imageComp.fileName = result.fileName;
								imageComp.filePath = result.filePath;

								// *** CRITICAL: Store image data for TextureSystem to use ***
								imageComp.imageData = result.data;

								// Update InputImageComponent if present
								if (mgr.HasComponent<InputImageComponent>(result.entityID)) {
									auto& inputComp = mgr.GetComponent<InputImageComponent>(result.entityID);

									inputComp.width = result.width;
									inputComp.height = result.height;
									inputComp.channels = result.channels;
									inputComp.fileName = result.fileName;
									inputComp.filePath = result.filePath;

									// Store image data safely in InputImageComponent
									inputComp.SetImageData(result.data, result.width, result.height, result.channels);

									std::cout << "[ImageSystem] Updated InputImageComponent for entity "
										<< result.entityID << std::endl;
								}

								std::cout << "[ImageSystem] Image data loaded successfully: " << result.fileName
									<< " (" << result.width << "x" << result.height << ")" << std::endl;

								// Notify callbacks - TextureSystem will handle texture creation
								// Note: imageData pointer ownership transfers to TextureSystem via callback
								NotifyImageAdded(result.entityID);
							}
							else {
								std::cerr << "[ImageSystem] Failed to load image: " << result.filePath << std::endl;
							}
						}
						else {
							// Entity was destroyed while loading
							std::cout << "[ImageSystem] Entity " << result.entityID
								<< " was destroyed during loading" << std::endl;
							if (result.data) {
								Utils::ImageUtils::FreeImageData(result.data);
							}
						}
					}
					catch (const std::exception& e) {
						std::cerr << "[ImageSystem] Exception processing completed load: " << e.what() << std::endl;
					}

					// Remove completed task
					it = pendingLoads.erase(it);
				}
				else {
					++it;
				}
			}
		}

		void CancelPendingLoad(EntityID entity) {
			std::lock_guard<std::mutex> lock(loadMutex);
			auto initialSize = pendingLoads.size();
			pendingLoads.erase(
				std::remove_if(pendingLoads.begin(), pendingLoads.end(),
					[entity](const LoadingTask& task) { return task.entityID == entity; }),
				pendingLoads.end());

			if (pendingLoads.size() < initialSize) {
				std::cout << "[ImageSystem] Cancelled pending load for entity: " << entity << std::endl;
			}
		}

		void NotifyImageAdded(EntityID entity) {
			for (const auto& callback : imageAddedCallbacks) {
				callback(entity);
			}
		}

		void NotifyImageRemoved(EntityID entity) {
			for (const auto& callback : imageRemovedCallbacks) {
				callback(entity);
			}
		}
	};
} // namespace ECS