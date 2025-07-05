/*
 * ImageSystem.hpp - RESTORED GUI Callback Mechanism
 * REQUIRED: ECS Systems SHOULD have callbacks to GUI classes for immediate notifications
 * GUI should both poll ECS for data AND receive immediate callbacks for responsive UI
 */

#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ThreadPool.hpp"
#include "ImageUtils.hpp"
#include "OpenGLUtils.hpp"
#include <GL/glew.h>
#include <memory>
#include <stb_image.h>
#include <stb_image_write.h>
#include <queue>
#include <mutex>
#include <future>
#include <functional>

namespace ECS {

	class ImageSystem : public BaseSystem {
	public:
		// Callback function types for GUI notifications
		using ImageLoadedCallback = std::function<void(EntityID entityID)>;
		using ImageRemovedCallback = std::function<void(EntityID entityID)>;

		struct LoadResult {
			bool success = false;
			unsigned char* data = nullptr;
			int width = 0;
			int height = 0;
			int channels = 0;
			std::string fileName;
			std::string filePath;
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
			std::cout << "[ImageSystem] Created - WITH GUI CALLBACKS SUPPORT" << std::endl;
		}

		~ImageSystem() override {
			std::cout << "[ImageSystem] Destructor - cleaning up textures..." << std::endl;

			// Clean up loaded textures
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
					UnloadImage(imageComp);
				}
			}

			std::cout << "[ImageSystem] Destructor complete" << std::endl;
		}

		void Start() override {
			std::cout << "[ImageSystem] Starting - processing existing entities..." << std::endl;

			// Load images for existing entities with ImageComponent
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
					if (!imageComp.filePath.empty()) {
						LoadImageAsync(entity, imageComp.filePath);
					}
				}
			}

			std::cout << "[ImageSystem] Start complete" << std::endl;
		}

		void Update(const float deltaT) override {
			ProcessCompletedLoads();
		}

		// GUI CALLBACK REGISTRATION METHODS
		void RegisterImageLoadedCallback(ImageLoadedCallback callback) {
			imageLoadedCallbacks.push_back(callback);
			std::cout << "[ImageSystem] Registered image loaded callback" << std::endl;
		}

		void RegisterImageRemovedCallback(ImageRemovedCallback callback) {
			imageRemovedCallbacks.push_back(callback);
			std::cout << "[ImageSystem] Registered image removed callback" << std::endl;
		}

		void ClearCallbacks() {
			imageLoadedCallbacks.clear();
			imageRemovedCallbacks.clear();
			std::cout << "[ImageSystem] Cleared all callbacks" << std::endl;
		}

		void SetImage(const EntityID entity, const std::string& filePath) {
			std::cout << "[ImageSystem] SetImage called for entity " << entity << ": " << filePath << std::endl;

			// Handle both regular ImageComponent and InputImageComponent
			if (mgr.HasComponent<ImageComponent>(entity)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

				// Unload current image if any
				if (imageComp.textureID != 0) {
					UnloadImage(imageComp);
				}

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
			std::cout << "[ImageSystem] RemoveImage called for entity " << entity << std::endl;

			if (mgr.HasComponent<ImageComponent>(entity)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

				// Cancel any pending load
				CancelPendingLoad(entity);

				// Clear input image data if this is an InputImageComponent
				if (mgr.HasComponent<InputImageComponent>(entity)) {
					auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
					inputComp.ClearImageData();
				}

				if (imageComp.textureID != 0) {
					UnloadImage(imageComp);
				}

				// NOTIFY GUI BEFORE DESTROYING THE ENTITY
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

		// Additional polling methods for GUI
		size_t GetImageEntityCount() const {
			return entities.size();
		}

		bool IsImageLoaded(EntityID entity) const {
			if (mgr.HasComponent<ImageComponent>(entity)) {
				const auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
				return imageComp.textureID != 0 && imageComp.width > 0 && imageComp.height > 0;
			}
			return false;
		}

		bool IsImageLoading(EntityID entity) const {
			std::lock_guard<std::mutex> lock(loadMutex);
			for (const auto& task : pendingLoads) {
				if (task.entityID == entity) {
					return true;
				}
			}
			return false;
		}

	private:
		std::vector<LoadingTask> pendingLoads;
		mutable std::mutex loadMutex; // Made mutable for const methods

		// GUI Callback storage
		std::vector<ImageLoadedCallback> imageLoadedCallbacks;
		std::vector<ImageRemovedCallback> imageRemovedCallbacks;

		void NotifyImageLoaded(EntityID entityID) {
			std::cout << "[ImageSystem] Notifying GUI callbacks: Image loaded for entity " << entityID << std::endl;
			for (auto& callback : imageLoadedCallbacks) {
				try {
					callback(entityID);
				}
				catch (const std::exception& e) {
					std::cerr << "[ImageSystem] Exception in image loaded callback: " << e.what() << std::endl;
				}
			}
		}

		void NotifyImageRemoved(EntityID entityID) {
			std::cout << "[ImageSystem] Notifying GUI callbacks: Image removed for entity " << entityID << std::endl;
			for (auto& callback : imageRemovedCallbacks) {
				try {
					callback(entityID);
				}
				catch (const std::exception& e) {
					std::cerr << "[ImageSystem] Exception in image removed callback: " << e.what() << std::endl;
				}
			}
		}

		void LoadImageAsync(EntityID entity, const std::string& filePath) {
			std::cout << "[ImageSystem] Starting async load for entity " << entity << ": " << filePath << std::endl;

			auto& ioPool = Utils::ThreadPoolManager::getInstance().getIOPool();

			// Create async task
			auto future = ioPool.submit([filePath]() -> LoadResult {
				LoadResult result;
				result.filePath = filePath;

				// Extract filename
				size_t lastSlash = filePath.find_last_of("/\\");
				if (lastSlash != std::string::npos) {
					result.fileName = filePath.substr(lastSlash + 1);
				}
				else {
					result.fileName = filePath;
				}

				// Load image data on I/O thread
				result.data = Utils::ImageUtils::LoadImageData(filePath, result.width, result.height, result.channels);
				result.success = (result.data != nullptr);

				return result;
			});

			// Store the task using emplace_back
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

						// Check if entity still exists
						if (mgr.HasComponent<ImageComponent>(it->entityID)) {
							auto& imageComp = mgr.GetComponent<ImageComponent>(it->entityID);

							if (result.success) {
								// Update base ImageComponent properties on main thread
								imageComp.width = result.width;
								imageComp.height = result.height;
								imageComp.channels = result.channels;
								imageComp.fileName = result.fileName;
								imageComp.filePath = result.filePath;

								// Create OpenGL texture on main thread using OpenGLUtils
								imageComp.textureID = Utils::OpenGLUtils::GenerateTexture(
									result.width, result.height, result.channels, result.data);

								// Update InputImageComponent if present
								if (mgr.HasComponent<InputImageComponent>(it->entityID)) {
									auto& inputComp = mgr.GetComponent<InputImageComponent>(it->entityID);

									// Update InputImageComponent properties
									inputComp.width = result.width;
									inputComp.height = result.height;
									inputComp.channels = result.channels;
									inputComp.fileName = result.fileName;
									inputComp.filePath = result.filePath;
									inputComp.textureID = imageComp.textureID; // Share the same texture

									// Store image data safely in InputImageComponent
									inputComp.SetImageData(result.data, result.width, result.height, result.channels);
									// Don't free result.data here - ownership transferred to InputImageComponent

									std::cout << "[ImageSystem] Updated InputImageComponent: " << result.fileName << " ("
										<< result.width << "x" << result.height << ")" << std::endl;
								}
								else {
									// Free data if not needed by InputImageComponent
									Utils::ImageUtils::FreeImageData(result.data);
								}

								std::cout << "[ImageSystem] Async loaded image: " << result.filePath << " ("
									<< result.width << "x" << result.height << ")" << std::endl;

								// NOTIFY GUI CALLBACKS THAT IMAGE WAS LOADED SUCCESSFULLY
								NotifyImageLoaded(it->entityID);
							}
							else {
								std::cerr << "[ImageSystem] Failed to async load image: " << result.filePath << std::endl;
							}
						}
						else {
							// Entity was destroyed while loading
							if (result.data) {
								Utils::ImageUtils::FreeImageData(result.data);
							}
							std::cout << "[ImageSystem] Entity " << it->entityID << " destroyed during loading" << std::endl;
						}
					}
					catch (const std::exception& e) {
						std::cerr << "[ImageSystem] Exception in async image loading: " << e.what() << std::endl;
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
			pendingLoads.erase(
				std::remove_if(pendingLoads.begin(), pendingLoads.end(),
					[entity](const LoadingTask& task) { return task.entityID == entity; }),
				pendingLoads.end());
		}

		void UnloadImage(ImageComponent& imageComp) {
			if (imageComp.textureID != 0) {
				Utils::OpenGLUtils::DeleteTexture(imageComp.textureID);
				imageComp.textureID = 0;
				imageComp.width = 0;
				imageComp.height = 0;
				imageComp.channels = 0;
				std::cout << "[ImageSystem] Unloaded image texture" << std::endl;
			}
		}
	};
} // namespace ECS