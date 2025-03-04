#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ImageManager.hpp"
#include <GL/glew.h>
#include <memory>
#include <functional>
#include <stb_image.h>
#include <stb_image_write.h>

namespace ECS {

    class ImageSystem : public BaseSystem {
    public:
        // Callback function types
        using ImageCallback = std::function<void(EntityID)>;

        ImageSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
            sysName = "Image_System";

            // Define which components this system operates on
            AddComponentSignature<ImageComponent>();

            // Initialize the image manager
            imageManager = std::make_unique<ImageManager>();
        }

        ~ImageSystem() override {
            Destroy();
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
            // Check if any images need to be loaded or updated
            for (auto entity : entities) {
                if (mgr.HasComponent<ImageComponent>(entity)) {
                    auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

                    // Check if image needs to be loaded (textureID is 0)
                    if (imageComp.textureID == 0 && !imageComp.filePath.empty()) {
                        LoadImage(imageComp);
                    }
                }
            }
        }

        void Render() override {
            // This system doesn't render directly; it manages images for other rendering systems
        }

        void Destroy() override {
            // Clean up images when the system is destroyed
            for (auto entity : entities) {
                if (mgr.HasComponent<ImageComponent>(entity)) {
                    auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
                    UnloadImage(imageComp);
                }
            }
        }

        // Load image for an entity
        void LoadImage(ImageComponent& imageComp) {
            if (!imageComp.filePath.empty()) {
                imageManager->ApplyImageToComponent(imageComp);
            }
        }

        // Unload image for an entity
        void UnloadImage(ImageComponent& imageComp) {
            if (imageComp.textureID != 0) {
                imageManager->ReleaseImage(imageComp.filePath);
                imageComp.textureID = 0;
            }
        }

        // Reload image (useful for hot-reloading during development)
        void ReloadImage(EntityID entity) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
                UnloadImage(imageComp);
                LoadImage(imageComp);
            }
        }

        // Set image filepath and load it
        void SetImage(EntityID entity, const std::string& filePath) {
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

        // Create an entity with an image loaded from a file
        EntityID CreateImageEntity(const std::string& filePath) {
            EntityID entity = mgr.AddNewEntity();
            auto& imageComp = mgr.AddComponent<ImageComponent>(entity);

            SetImage(entity, filePath);

            // Notify listeners that an image was added
            for (const auto& callback : imageAddedCallbacks) {
                callback(entity);
            }

            return entity;
        }

        // Load multiple images and return the created entities
        std::vector<EntityID> LoadImages(const std::vector<std::string>& filePaths) {
            std::vector<EntityID> entities;
            for (const auto& path : filePaths) {
                EntityID entity = CreateImageEntity(path);
                entities.push_back(entity);
            }
            return entities;
        }

        // Save an image to a file
        bool SaveImage(EntityID entity, const std::string& filePath) {
            if (!mgr.HasComponent<ImageComponent>(entity)) {
                return false;
            }

            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

            // If we don't have image data in memory, we need to reload it
            if (!imageComp.imageData) {
                // Load image data without affecting the GPU texture
                imageComp.imageData = stbi_load(imageComp.filePath.c_str(),
                    &imageComp.width,
                    &imageComp.height,
                    &imageComp.channels,
                    0);
                if (!imageComp.imageData) {
                    return false;
                }
            }

            // Determine the file format based on extension
            std::string ext = filePath.substr(filePath.find_last_of(".") + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            bool success = false;
            if (ext == "png") {
                success = stbi_write_png(filePath.c_str(), imageComp.width, imageComp.height,
                    imageComp.channels, imageComp.imageData, 0);
            }
            else if (ext == "jpg" || ext == "jpeg") {
                success = stbi_write_jpg(filePath.c_str(), imageComp.width, imageComp.height,
                    imageComp.channels, imageComp.imageData, 90); // 90 quality
            }
            else if (ext == "bmp") {
                success = stbi_write_bmp(filePath.c_str(), imageComp.width, imageComp.height,
                    imageComp.channels, imageComp.imageData);
            }
            else if (ext == "tga") {
                success = stbi_write_tga(filePath.c_str(), imageComp.width, imageComp.height,
                    imageComp.channels, imageComp.imageData);
            }

            // Update the component's filepath if save was successful
            if (success) {
                imageComp.filePath = filePath;
                // Extract filename from path
                size_t lastSlash = filePath.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    imageComp.fileName = filePath.substr(lastSlash + 1);
                }
                else {
                    imageComp.fileName = filePath;
                }
            }

            return success;
        }

        // Get ImageManager instance
        ImageManager* GetImageManager() {
            return imageManager.get();
        }

        // Register callbacks
        void RegisterImageAddedCallback(const ImageCallback& callback) {
            imageAddedCallbacks.push_back(callback);
        }

        void RegisterImageRemovedCallback(const ImageCallback& callback) {
            imageRemovedCallbacks.push_back(callback);
        }

    private:
        std::unique_ptr<ImageManager> imageManager;
        std::vector<ImageCallback> imageAddedCallbacks;
        std::vector<ImageCallback> imageRemovedCallbacks;
    };

} // namespace ECS