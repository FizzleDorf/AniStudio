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

namespace ECS {

	class ImageSystem : public BaseSystem {
	public:
		// Callback function types
		using ImageCallback = std::function<void(EntityID)>;

		ImageSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr) { // Use 2 threads for image operations
			sysName = "ImageSystem";

			// Define which components this system operates on
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
					LoadImage(imageComp);
				}
			}
		}

		// TODO: async the image IO and join in the update
		void Update(const float deltaT) override {}

		// Register callbacks for image events
		void RegisterImageAddedCallback(const ImageCallback& callback) {
			imageAddedCallbacks.push_back(callback);
		}

		void RegisterImageRemovedCallback(const ImageCallback& callback) {
			imageRemovedCallbacks.push_back(callback);
		}

		// Set image filepath and load it
		void SetImage(const EntityID entity, const std::string& filePath) {
			if (mgr.HasComponent<ImageComponent>(entity)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

				// Unload current image if any
				if (imageComp.textureID != 0) {
					UnloadImage(imageComp);
				}

				// Set new path and load
				imageComp.filePath = filePath;

				// Extract filename from path
				size_t lastSlash = filePath.find_last_of("/\\");
				if (lastSlash != std::string::npos) {
					imageComp.fileName = filePath.substr(lastSlash + 1);
				}
				else {
					imageComp.fileName = filePath;
				}

				LoadImage(imageComp);
				NotifyImageAdded(entity);
			}
		}

		// Remove an image entity
		void RemoveImage(const EntityID entity) {
			if (mgr.HasComponent<ImageComponent>(entity)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

				// Unload image
				if (imageComp.textureID != 0) {
					UnloadImage(imageComp);
				}

				// Notify before removing
				NotifyImageRemoved(entity);

				// Remove the entity
				mgr.DestroyEntity(entity);
			}
		}

		// Get a list of all entities with an ImageComponent
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

		// Notify image callbacks
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

		// Load an image synchronously (used by Start and SetImage)
		void LoadImage(ImageComponent& imageComp) {
			if (imageComp.filePath.empty()) {
				return;
			}

			// Load image data
			int width, height, channels;
			unsigned char* data = Utils::ImageUtils::LoadImageData(imageComp.filePath, width, height, channels);

			if (data) {
				// Store image dimensions
				imageComp.width = width;
				imageComp.height = height;
				imageComp.channels = channels;

				// Create OpenGL texture
				imageComp.textureID = Utils::ImageUtils::GenerateTexture(width, height, channels, data);

				// Free image data
				Utils::ImageUtils::FreeImageData(data);

				std::cout << "Loaded image: " << imageComp.filePath << " (" << width << "x" << height << ")" << std::endl;
			}
			else {
				std::cerr << "Failed to load image: " << imageComp.filePath << std::endl;
			}
		}

		// Unload an image
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