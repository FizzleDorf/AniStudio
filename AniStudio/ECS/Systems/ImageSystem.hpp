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
            : BaseSystem(entityMgr),
            threadPool(std::make_unique<Utils::ThreadPool>(2)) { // Use 2 threads for image operations
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
            // Process pending texture operations (these must happen on the main thread)
            ProcessPendingDeletions();

            for (auto entity : entities) {
                if (mgr.HasComponent<ImageComponent>(entity)) {
                    auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

                    // Only generate textures for components with imageData but no textureID
                    if (imageComp.textureID == 0 && imageComp.imageData) {
                        // Validate dimensions and data before attempting texture creation
                        if (imageComp.width <= 0 || imageComp.height <= 0 || imageComp.channels <= 0 || imageComp.channels > 4) {
                            std::cerr << "Entity " << entity << " has invalid image dimensions: "
                                << imageComp.width << "x" << imageComp.height << "x" << imageComp.channels
                                << " - Cannot create texture" << std::endl;
                            continue;
                        }

                        try {
                            // Generate texture from existing data
                            std::cout << "Generating texture for entity " << entity << std::endl;

                            GLuint newTextureID = Utils::ImageUtils::GenerateTexture(
                                imageComp.width, imageComp.height, imageComp.channels, imageComp.imageData);

                            if (newTextureID != 0) {
                                // Assign the new texture ID
                                imageComp.textureID = newTextureID;
                                std::cout << "Successfully generated texture " << newTextureID << " for entity " << entity << std::endl;

                                // Notify that an image was added (texture created)
                                // NotifyImageAdded(entity);
                            }
                            else {
                                std::cerr << "Failed to generate texture for entity " << entity << std::endl;
                            }
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Exception when generating texture for entity " << entity << ": " << e.what() << std::endl;
                        }
                        catch (...) {
                            std::cerr << "Unknown exception when generating texture for entity " << entity << std::endl;
                        }
                    }
                }
            }
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

        // Queue image loading task - Note: The component must already have filePath set
        void QueueLoadImage(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            // Create load task
            class LoadImageTask : public Utils::Task {
            public:
                LoadImageTask(ImageSystem* system, EntityID entity)
                    : system(system), entityID(entity) {
                }

                void execute() override {
                    if (!isCancelled()) {
                        system->LoadImageAsync(entityID);
                        markDone();
                    }
                }

            private:
                ImageSystem* system;
                EntityID entityID;
            };

            // Add to thread pool
            auto task = std::make_shared<LoadImageTask>(this, entity);
            threadPool->addTask(task);
        }

        // Queue image saving task - Note: The component must already have a valid imageData
        void QueueSaveImage(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            // Create save task
            class SaveImageTask : public Utils::Task {
            public:
                SaveImageTask(ImageSystem* system, EntityID entity)
                    : system(system), entityID(entity) {
                }

                void execute() override {
                    if (!isCancelled()) {
                        system->SaveImageAsync(entityID);
                        markDone();
                    }
                }

            private:
                ImageSystem* system;
                EntityID entityID;
            };

            // Add to thread pool
            auto task = std::make_shared<SaveImageTask>(this, entity);
            threadPool->addTask(task);
        }

        // Queue image removal task
        void QueueRemoveImage(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            // Create remove task
            class RemoveImageTask : public Utils::Task {
            public:
                RemoveImageTask(ImageSystem* system, EntityID entity)
                    : system(system), entityID(entity) {
                }

                void execute() override {
                    if (!isCancelled()) {
                        system->RemoveImageAsync(entityID);
                        markDone();
                    }
                }

            private:
                ImageSystem* system;
                EntityID entityID;
            };

            // Add to thread pool
            auto task = std::make_shared<RemoveImageTask>(this, entity);
            threadPool->addTask(task);
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

        // Async image loading implementation
        void LoadImageAsync(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

            if (imageComp.filePath.empty()) {
                std::cerr << "Cannot load image: No file path provided for entity " << entity << std::endl;
                return;
            }

            // Load image data
            imageComp.imageData = Utils::ImageUtils::LoadImageData(imageComp.filePath, imageComp.width, imageComp.height, imageComp.channels);
        }

        // Async image saving implementation
        void SaveImageAsync(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

            // Check if image data and path exist
            if (imageComp.imageData == nullptr) {
                std::cerr << "Cannot save image: No image data available for entity " << entity << std::endl;
                return;
            }

            if (imageComp.filePath.empty()) {
                std::cerr << "Cannot save image: No file path provided for entity " << entity << std::endl;
                return;
            }

            // Save directly from the image data in the component
            bool success = Utils::ImageUtils::SaveImage(
                imageComp.filePath,
                imageComp.width,
                imageComp.height,
                imageComp.channels,
                imageComp.imageData
            );

            if (success) {
                std::cout << "Successfully saved image to: " << imageComp.filePath << std::endl;
            }
            else {
                std::cerr << "Failed to save image to: " << imageComp.filePath << std::endl;
            }
        }

        // Async image removal implementation
        void RemoveImageAsync(EntityID entity) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return;
            }

            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

            // Schedule texture deletion on main thread
            if (imageComp.textureID != 0) {
                std::lock_guard<std::mutex> lock(pendingDeletionMutex);
                pendingDeletions.push({ entity, imageComp.textureID });
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

        // Thread pool for asynchronous image operations
        std::unique_ptr<Utils::ThreadPool> threadPool;

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