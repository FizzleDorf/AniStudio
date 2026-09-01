#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "TextureComponent.hpp"
#include "VideoSystem.hpp"
#include "ImageUtils.hpp"
#include "OpenGLUtils.hpp"
#include "DragDropUtils.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <functional>
#include <cstdlib>

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
            AddComponentSignature<TextureComponent>();
        }

        ~TextureSystem() override {
            for (auto entity : entities) {
                if (mgr.HasComponent<ImageComponent>(entity)) {
                    auto& imgComp = mgr.GetComponent<ImageComponent>(entity);
                    DeleteTexture(imgComp);
                }
                if (mgr.HasComponent<TextureComponent>(entity)) {
                    auto& texComp = mgr.GetComponent<TextureComponent>(entity);
                    DeleteTexture(texComp);
                }
            }
        }

        void Start() override {
            auto videoSystem = mgr.GetSystem<VideoSystem>();
            if (videoSystem) {
                videoSystem->RegisterVideoRemovedCallback([this](EntityID entity) {
                    RemoveTexture(entity);
                    });
            }
        }

        void Update(const float deltaT) override {
            CreatePendingTextures();
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

        void CancelPendingRequests(EntityID entityID) {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::queue<TextureCreationRequest> newQueue;
            while (!textureQueue.empty()) {
                auto request = textureQueue.front();
                textureQueue.pop();
                if (request.entityID == entityID) {
                    if (request.isVideo) {
                        if (request.targetTexture && *request.targetTexture != 0) {
                            glDeleteTextures(1, request.targetTexture);
                            *request.targetTexture = 0;
                        }
                        if (request.imageData) {
                            free(request.imageData);
                        }
                    }
                    else {
                        if (request.imageData) {
                            Utils::ImageUtils::FreeImageData(request.imageData);
                        }
                    }
                }
                else {
                    newQueue.push(request);
                }
            }
            textureQueue.swap(newQueue);
            m_needsTextureCreation = !textureQueue.empty();
        }

        void CreatePendingTextures() {
            std::lock_guard<std::mutex> lock(queueMutex);

            if (textureQueue.empty()) {
                m_needsTextureCreation = false;
                return;
            }

            GLFWwindow* currentContext = glfwGetCurrentContext();
            if (!currentContext) {
                GLFWwindow* window = static_cast<GLFWwindow*>(GUI::DragDrop::GetWindowHandle());
                if (window) {
                    glfwMakeContextCurrent(window);
                    currentContext = window;
                }
                if (!currentContext) return;
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
                    if (request.isVideo) {
                        if (request.targetTexture && *request.targetTexture != 0) {
                            glDeleteTextures(1, request.targetTexture);
                            *request.targetTexture = 0;
                        }
                        if (request.imageData) {
                            free(request.imageData);
                        }
                    }
                    else {
                        if (request.imageData) {
                            Utils::ImageUtils::FreeImageData(request.imageData);
                        }
                    }
                    continue;
                }

                if (request.isVideo) {
                    if (!request.targetTexture) {
                        if (request.imageData) {
                            free(request.imageData);
                        }
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
                    if (request.imageData) {
                        free(request.imageData);
                    }
                }
                else {
                    if (!mgr.HasComponent<ImageComponent>(request.entityID)) {
                        if (request.imageData) {
                            Utils::ImageUtils::FreeImageData(request.imageData);
                        }
                        continue;
                    }
                    auto& imgComp = mgr.GetComponent<ImageComponent>(request.entityID);
                    if (imgComp.textureID != 0) {
                        DeleteTexture(imgComp);
                    }
                    imgComp.textureID = Utils::OpenGLUtils::GenerateTexture(
                        request.width, request.height, request.channels, request.imageData
                    );
                    if (imgComp.textureID != 0) {
                        imgComp.width = request.width;
                        imgComp.height = request.height;
                        imgComp.channels = request.channels;
                    }
                    if (request.imageData) {
                        Utils::ImageUtils::FreeImageData(request.imageData);
                    }
                }
            }
            m_needsTextureCreation = false;
        }

        void RemoveTexture(EntityID entityID) {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                auto& imgComp = mgr.GetComponent<ImageComponent>(entityID);
                DeleteTexture(imgComp);
            }
            if (mgr.HasComponent<TextureComponent>(entityID)) {
                auto& texComp = mgr.GetComponent<TextureComponent>(entityID);
                DeleteTexture(texComp);
            }
            CancelPendingRequests(entityID);
        }

        GLuint GetTextureID(EntityID entityID) const {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                return mgr.GetComponent<ImageComponent>(entityID).textureID;
            }
            if (mgr.HasComponent<TextureComponent>(entityID)) {
                return mgr.GetComponent<TextureComponent>(entityID).textureID;
            }
            return 0;
        }

        bool HasValidTexture(EntityID entityID) const {
            GLuint texID = GetTextureID(entityID);
            return texID != 0 && glIsTexture(texID);
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

        void DeleteTexture(ImageComponent& imgComp) {
            if (imgComp.textureID != 0) {
                GLFWwindow* currentContext = glfwGetCurrentContext();
                if (currentContext) {
                    Utils::OpenGLUtils::DeleteTexture(imgComp.textureID);
                }
                imgComp.textureID = 0;
                imgComp.width = 0;
                imgComp.height = 0;
                imgComp.channels = 0;
            }
        }

        void DeleteTexture(TextureComponent& texComp) {
            if (texComp.textureID != 0) {
                GLFWwindow* currentContext = glfwGetCurrentContext();
                if (currentContext) {
                    Utils::OpenGLUtils::DeleteTexture(texComp.textureID);
                }
                texComp.textureID = 0;
                texComp.width = 0;
                texComp.height = 0;
                texComp.channels = 0;
                texComp.needsUpdate = false;
            }
        }
    };

}