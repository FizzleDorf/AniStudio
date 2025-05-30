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

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ThreadPool.hpp"
#include "ImageUtils.hpp"
#include <GL/glew.h>
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
		};

		struct LoadingTask {
			EntityID entityID;
			std::string filePath;
			std::future<LoadResult> future;

			// Default constructor
			LoadingTask() = default;

			// Move constructor
			LoadingTask(LoadingTask&& other) noexcept
				: entityID(other.entityID)
				, filePath(std::move(other.filePath))
				, future(std::move(other.future)) {}

			// Move assignment
			LoadingTask& operator=(LoadingTask&& other) noexcept {
				if (this != &other) {
					entityID = other.entityID;
					filePath = std::move(other.filePath);
					future = std::move(other.future);
				}
				return *this;
			}

			// Delete copy operations
			LoadingTask(const LoadingTask&) = delete;
			LoadingTask& operator=(const LoadingTask&) = delete;
		};

		ImageSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr) {
			sysName = "ImageSystem";
			AddComponentSignature<ImageComponent>();
		}

		~ImageSystem() override {
			// Clean up loaded textures
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
					UnloadImage(imageComp);
				}
			}
		}

		void Start() override {
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

				NotifyImageRemoved(entity);
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

								// Create OpenGL texture on main thread
								imageComp.textureID = Utils::ImageUtils::GenerateTexture(
									result.width, result.height, result.channels, result.data);

								// CRITICAL FIX: Update InputImageComponent if present
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

									std::cout << "Updated InputImageComponent: " << result.fileName << " ("
										<< result.width << "x" << result.height << ")" << std::endl;
								}
								else {
									// Free data if not needed by InputImageComponent
									Utils::ImageUtils::FreeImageData(result.data);
								}

								std::cout << "Async loaded image: " << result.filePath << " ("
									<< result.width << "x" << result.height << ")" << std::endl;

								NotifyImageAdded(it->entityID);
							}
							else {
								std::cerr << "Failed to async load image: " << result.filePath << std::endl;
							}
						}
						else {
							// Entity was destroyed while loading
							if (result.data) {
								Utils::ImageUtils::FreeImageData(result.data);
							}
						}
					}
					catch (const std::exception& e) {
						std::cerr << "Exception in async image loading: " << e.what() << std::endl;
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

		void UnloadImage(ImageComponent& imageComp) {
			if (imageComp.textureID != 0) {
				Utils::ImageUtils::DeleteTexture(imageComp.textureID);
				imageComp.textureID = 0;
				imageComp.width = 0;
				imageComp.height = 0;
				imageComp.channels = 0;
			}
		}
	};
} // namespace ECS