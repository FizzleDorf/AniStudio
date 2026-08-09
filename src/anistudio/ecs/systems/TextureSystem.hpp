#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ImageUtils.hpp"
#include "OpenGLUtils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <functional>

namespace ECS {

    struct TextureCreationRequest {
        EntityID entityID;
        unsigned char* imageData;
        int width;
        int height;
        int channels;
        bool isVideo;
        GLuint* targetTexture;
    };

    class TextureSystem : public BaseSystem {
    public:
        using VideoTextureCallback = std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)>;

        TextureSystem(EntityManager& entityMgr)
            : BaseSystem(entityMgr), m_needsTextureCreation(false) {
            sysName = "TextureSystem";
            AddComponentSignature<ImageComponent>();
        }

        ~TextureSystem() override {
            for (auto entity : entities) {
                if (mgr.HasComponent<ImageComponent>(entity)) {
                    auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
                    DeleteTexture(imageComp);
                }
            }
        }

        void Start() override {}

        void Update(const float deltaT) override {
            std::lock_guard<std::mutex> lock(queueMutex);
            m_needsTextureCreation = !textureQueue.empty();
        }

        void QueueTextureCreation(EntityID entityID, unsigned char* imageData, int width, int height, int channels) {
            std::lock_guard<std::mutex> lock(queueMutex);
            TextureCreationRequest request;
            request.entityID = entityID;
            request.imageData = imageData;
            request.width = width;
            request.height = height;
            request.channels = channels;
            request.isVideo = false;
            request.targetTexture = nullptr;
            textureQueue.push(request);
            m_needsTextureCreation = true;
        }

        void QueueVideoTextureCreation(EntityID entityID, unsigned char* data, int width, int height, int channels, GLuint* targetTexture) {
            std::lock_guard<std::mutex> lock(queueMutex);
            TextureCreationRequest request;
            request.entityID = entityID;
            request.imageData = data;
            request.width = width;
            request.height = height;
            request.channels = channels;
            request.isVideo = true;
            request.targetTexture = targetTexture;
            textureQueue.push(request);
            m_needsTextureCreation = true;
        }

        void CreatePendingTextures() {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (textureQueue.empty()) {
                m_needsTextureCreation = false;
                return;
            }

            GLFWwindow* currentContext = glfwGetCurrentContext();
            if (!currentContext) {
                return;
            }

            glGetError();
            GLint textureUnits;
            glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                return;
            }

            while (!textureQueue.empty()) {
                TextureCreationRequest request = textureQueue.front();
                textureQueue.pop();

                if (!mgr.IsEntityValid(request.entityID)) {
                    if (request.imageData && !request.isVideo) {
                        Utils::ImageUtils::FreeImageData(request.imageData);
                    }
                    continue;
                }

                if (request.isVideo) {
                    if (!request.targetTexture) {
                        continue;
                    }
                    if (*request.targetTexture != 0) {
                        glDeleteTextures(1, request.targetTexture);
                        *request.targetTexture = 0;
                    }
                    GLuint texID = Utils::OpenGLUtils::GenerateTexture(
                        request.width, request.height, request.channels, request.imageData
                    );
                    if (texID != 0) {
                        *request.targetTexture = texID;
                    }
                }
                else {
                    if (!mgr.HasComponent<ImageComponent>(request.entityID)) {
                        if (request.imageData) {
                            Utils::ImageUtils::FreeImageData(request.imageData);
                        }
                        continue;
                    }
                    auto& imageComp = mgr.GetComponent<ImageComponent>(request.entityID);
                    if (imageComp.textureID != 0) {
                        DeleteTexture(imageComp);
                    }
                    imageComp.textureID = Utils::OpenGLUtils::GenerateTexture(
                        request.width, request.height, request.channels, request.imageData
                    );
                    if (imageComp.textureID != 0) {
                        imageComp.width = request.width;
                        imageComp.height = request.height;
                        imageComp.channels = request.channels;
                    }
                }
            }
            m_needsTextureCreation = false;
        }

        void RemoveTexture(EntityID entityID) {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                auto& imageComp = mgr.GetComponent<ImageComponent>(entityID);
                DeleteTexture(imageComp);
            }
        }

        GLuint GetTextureID(EntityID entityID) const {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                return mgr.GetComponent<ImageComponent>(entityID).textureID;
            }
            return 0;
        }

        bool HasValidTexture(EntityID entityID) const {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                GLuint textureID = mgr.GetComponent<ImageComponent>(entityID).textureID;
                return textureID != 0 && glIsTexture(textureID);
            }
            return false;
        }

        bool HasPendingTextures() const { return m_needsTextureCreation; }

        void RegisterVideoTextureCallback(const VideoTextureCallback& callback) {
            m_videoTextureCallback = callback;
        }

        void QueueVideoTexture(EntityID entityID, unsigned char* data, int width, int height, int channels, GLuint* targetTexture) {
            if (m_videoTextureCallback) {
                m_videoTextureCallback(entityID, data, width, height, channels, targetTexture);
            }
            else {
                QueueVideoTextureCreation(entityID, data, width, height, channels, targetTexture);
            }
        }

    private:
        std::queue<TextureCreationRequest> textureQueue;
        std::mutex queueMutex;
        bool m_needsTextureCreation;
        VideoTextureCallback m_videoTextureCallback;

        void DeleteTexture(ImageComponent& imageComp) {
            if (imageComp.textureID != 0) {
                GLFWwindow* currentContext = glfwGetCurrentContext();
                if (currentContext) {
                    Utils::OpenGLUtils::DeleteTexture(imageComp.textureID);
                }
                imageComp.textureID = 0;
                imageComp.width = 0;
                imageComp.height = 0;
                imageComp.channels = 0;
            }
        }
    };

} // namespace ECS