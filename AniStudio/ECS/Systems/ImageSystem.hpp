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
		/*using ImageCallback = std::function<void(EntityID)>;*/

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

		void Update(const float deltaT) override {
			ProcessPendingDeletions();
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

		// Process pending texture deletions (must be called on main thread)
		void ProcessPendingDeletions() {
			std::lock_guard<std::mutex> lock(pendingDeletionMutex);

			while (!pendingDeletions.empty()) {
				auto& pending = pendingDeletions.front();

				if (mgr.HasComponent<ImageComponent>(pending.entityID)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(pending.entityID);

					// Delete texture if it matches the pending one
					if (imageComp.textureID == pending.textureID) {
						Utils::ImageUtils::DeleteTexture(imageComp.textureID);
						imageComp.textureID = 0;
						imageComp.width = 0;
						imageComp.height = 0;
						imageComp.channels = 0;

						// Notify
						// NotifyImageRemoved(pending.entityID);
					}
				}

				pendingDeletions.pop();
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

		// Structure for pending texture deletion
		struct PendingDeletion {
			EntityID entityID;
			GLuint textureID;
		};

		// Queue for pending texture deletions (must be processed on main thread)
		std::mutex pendingDeletionMutex;
		std::queue<PendingDeletion> pendingDeletions;
	};
} // namespace ECS