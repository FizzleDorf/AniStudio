#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "OpenGLUtils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <queue>
#include <mutex>

namespace ECS {

	struct TextureCreationRequest {
		EntityID entityID;
		unsigned char* imageData;
		int width;
		int height;
		int channels;
	};

	class TextureSystem : public BaseSystem {
	public:
		TextureSystem(EntityManager& entityMgr)
			: BaseSystem(entityMgr) {
			sysName = "TextureSystem";
			AddComponentSignature<ImageComponent>();
			std::cout << "[TextureSystem] Constructor - Ready for texture creation" << std::endl;
		}

		~TextureSystem() override {
			std::cout << "[TextureSystem] Destructor - cleaning up textures" << std::endl;

			// Clean up all textures - trust that context is current during shutdown
			for (auto entity : entities) {
				if (mgr.HasComponent<ImageComponent>(entity)) {
					auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
					DeleteTexture(imageComp);
				}
			}
		}

		void Start() override {
			std::cout << "[TextureSystem] Started" << std::endl;
		}

		void Update(const float deltaT) override {
			// Don't process textures here - moved to ProcessGLOperations()
		}

		// NEW: Call this from main thread during render phase when OpenGL context is current
		void ProcessGLOperations() {
			ProcessPendingTextureCreations();
		}

		// Called by ImageSystem callbacks to queue texture creation
		void QueueTextureCreation(EntityID entityID, unsigned char* imageData, int width, int height, int channels) {
			std::lock_guard<std::mutex> lock(queueMutex);

			TextureCreationRequest request;
			request.entityID = entityID;
			request.imageData = imageData;
			request.width = width;
			request.height = height;
			request.channels = channels;

			textureQueue.push(request);

			std::cout << "[TextureSystem] Queued texture creation for entity " << entityID
				<< " (" << width << "x" << height << ")" << std::endl;
		}

		// Called to delete texture for an entity
		void RemoveTexture(EntityID entityID) {
			if (mgr.HasComponent<ImageComponent>(entityID)) {
				auto& imageComp = mgr.GetComponent<ImageComponent>(entityID);
				DeleteTexture(imageComp);
				std::cout << "[TextureSystem] Removed texture for entity " << entityID << std::endl;
			}
		}

		// Get texture ID for an entity
		GLuint GetTextureID(EntityID entityID) const {
			if (mgr.HasComponent<ImageComponent>(entityID)) {
				return mgr.GetComponent<ImageComponent>(entityID).textureID;
			}
			return 0;
		}

		// Check if entity has a valid texture
		bool HasValidTexture(EntityID entityID) const {
			if (mgr.HasComponent<ImageComponent>(entityID)) {
				GLuint textureID = mgr.GetComponent<ImageComponent>(entityID).textureID;
				return textureID != 0 && glIsTexture(textureID);
			}
			return false;
		}

	private:
		std::queue<TextureCreationRequest> textureQueue;
		std::mutex queueMutex;

		void ProcessPendingTextureCreations() {
			std::lock_guard<std::mutex> lock(queueMutex);

			if (textureQueue.empty()) return;

			// VERIFY we have a valid OpenGL context
			GLFWwindow* currentContext = glfwGetCurrentContext();
			if (!currentContext) {
				std::cerr << "[TextureSystem] ERROR: No OpenGL context current! Deferring texture creation." << std::endl;
				return;
			}

			// Test OpenGL context validity
			GLint textureUnits;
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				std::cerr << "[TextureSystem] ERROR: OpenGL context not valid! Error: " << error << std::endl;
				return;
			}

			std::cout << "[TextureSystem] Processing " << textureQueue.size() << " texture creation requests with valid OpenGL context" << std::endl;

			while (!textureQueue.empty()) {
				TextureCreationRequest request = textureQueue.front();
				textureQueue.pop();

				std::cout << "[TextureSystem] Creating texture for entity " << request.entityID
					<< " (" << request.width << "x" << request.height << ", "
					<< request.channels << " channels)" << std::endl;

				// Verify entity still exists and has ImageComponent
				if (!mgr.IsEntityValid(request.entityID) ||
					!mgr.HasComponent<ImageComponent>(request.entityID)) {
					std::cout << "[TextureSystem] Entity " << request.entityID
						<< " no longer valid, skipping texture creation" << std::endl;
					continue;
				}

				auto& imageComp = mgr.GetComponent<ImageComponent>(request.entityID);

				// Delete old texture if it exists
				if (imageComp.textureID != 0) {
					DeleteTexture(imageComp);
				}

				// Create the texture - NOW WITH GUARANTEED OPENGL CONTEXT
				imageComp.textureID = Utils::OpenGLUtils::GenerateTexture(
					request.width,
					request.height,
					request.channels,
					request.imageData
				);

				if (imageComp.textureID != 0) {
					std::cout << "[TextureSystem] Texture created successfully (ID: "
						<< imageComp.textureID << ") for entity " << request.entityID << std::endl;

					// Update component with texture dimensions
					imageComp.width = request.width;
					imageComp.height = request.height;
					imageComp.channels = request.channels;
				}
				else {
					std::cerr << "[TextureSystem] ERROR: Failed to create OpenGL texture for entity "
						<< request.entityID << std::endl;
				}
			}
		}

		void DeleteTexture(ImageComponent& imageComp) {
			if (imageComp.textureID != 0) {
				// Trust that context is current during cleanup
				Utils::OpenGLUtils::DeleteTexture(imageComp.textureID);
				std::cout << "[TextureSystem] Deleted texture ID: " << imageComp.textureID << std::endl;
				imageComp.textureID = 0;
				imageComp.width = 0;
				imageComp.height = 0;
				imageComp.channels = 0;
			}
		}
	};

} // namespace ECS