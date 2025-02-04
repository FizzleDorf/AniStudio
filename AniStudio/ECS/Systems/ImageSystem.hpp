#ifndef IMAGESYSTEM_HPP
#define IMAGESYSTEM_HPP

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include <GL/glew.h>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <filesystem>

namespace ECS {

    class ImageSystem : public BaseSystem {
    public:
        ImageSystem(EntityManager& entityMgr) : BaseSystem(entityMgr) {
            sysName = "ImageSystem";
            // Register component signature to track image entities
            AddComponentSignature<ImageComponent>();
        }

        ~ImageSystem() {
            // Clean up any resources when system is destroyed
            for (auto& image : loadedImages) {
                if (image.imageData) {
                    stbi_image_free(image.imageData);
                    image.imageData = nullptr;
                }
                if (image.textureID) {
                    glDeleteTextures(1, &image.textureID);
                    image.textureID = 0;
                }
            }
        }

        void Start() override {
            std::cout << "ImageSystem initialized" << std::endl;
        }

        void Update(const float deltaT) override {
            // Process any queued operations
            ProcessImageQueue();
        }

        // Get all loaded images
        std::vector<ImageComponent>& GetImages() {
            return loadedImages;
        }

        // Thread-safe image collection management
        void AddImage(const ImageComponent& image) {
            std::lock_guard<std::mutex> lock(imageMutex);
            loadedImages.push_back(image);
        }

        void AddImage(const EntityID entity) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                std::lock_guard<std::mutex> lock(imageMutex);
                loadedImages.push_back(mgr.GetComponent<ImageComponent>(entity));
            }
        }

        void RemoveImage(const size_t index) {
            std::lock_guard<std::mutex> lock(imageMutex);
            if (index >= loadedImages.size()) {
                throw std::out_of_range("Image index out of range");
            }

            // Clean up resources
            if (loadedImages[index].imageData) {
                stbi_image_free(loadedImages[index].imageData);
            }
            if (loadedImages[index].textureID) {
                glDeleteTextures(1, &loadedImages[index].textureID);
            }

            loadedImages.erase(loadedImages.begin() + index);
        }

        ImageComponent& GetImage(const size_t index) {
            std::lock_guard<std::mutex> lock(imageMutex);
            if (index >= loadedImages.size()) {
                throw std::out_of_range("Image index out of range");
            }
            return loadedImages[index];
        }

        // Queue operations for thread safety
        void QueueSaveImage(const EntityID entityID) {
            std::lock_guard<std::mutex> lock(queueMutex);
            SaveOperation op;
            op.type = OperationType::SaveEntityImage;
            op.entityID = entityID;
            imageOperationQueue.push(op);
        }

        void QueueSaveImage(const ImageComponent& imageComp) {
            std::lock_guard<std::mutex> lock(queueMutex);
            SaveOperation op;
            op.type = OperationType::SaveImage;
            op.component = imageComp;
            imageOperationQueue.push(op);
        }

        // Direct operations (use with caution in multithreaded context)
        void SaveImage(const EntityID entityID) {
            const auto& imageComp = mgr.GetComponent<ImageComponent>(entityID);
            HandleSave(imageComp);
        }

        void SaveImage(const ImageComponent& imageComp) {
            HandleSave(imageComp);
        }

        // Create/load texture functions
        GLuint CreateTextureFromImage(const ImageComponent& image) {
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            GLenum format = (image.channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.imageData);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);

            return textureID;
        }

        void LoadImageFromFile(ImageComponent& image) {
            // Clean up any existing image data
            if (image.imageData) {
                stbi_image_free(image.imageData);
                image.imageData = nullptr;
            }

            // Load the new image
            image.imageData = stbi_load(image.filePath.c_str(), &image.width, &image.height, &image.channels, 0);
            if (!image.imageData) {
                throw std::runtime_error("Failed to load image: " + image.filePath);
            }

            // Create texture
            if (image.textureID) {
                glDeleteTextures(1, &image.textureID);
            }
            image.textureID = CreateTextureFromImage(image);
        }

        // Load an image from file and create an entity for it
        EntityID CreateImageEntity(const std::string& filePath) {
            EntityID newEntity = mgr.AddNewEntity();
            mgr.AddComponent<ImageComponent>(newEntity);
            auto& imageComp = mgr.GetComponent<ImageComponent>(newEntity);

            // Extract filename from path
            std::filesystem::path path(filePath);
            imageComp.fileName = path.filename().string();
            imageComp.filePath = filePath;

            try {
                LoadImageFromFile(imageComp);
                AddImage(imageComp);
                return newEntity;
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading image: " << e.what() << std::endl;
                mgr.DestroyEntity(newEntity);
                return 0; // Return invalid entity ID
            }
        }

    private:
        enum class FileType { PNG, JPG, BMP, TGA, HDR, Unsupported };
        enum class OperationType { SaveEntityImage, SaveImage, LoadImage };

        struct SaveOperation {
            OperationType type;
            EntityID entityID = 0;
            ImageComponent component;
        };

        std::vector<ImageComponent> loadedImages;
        std::queue<SaveOperation> imageOperationQueue;
        std::mutex imageMutex;  // For protecting the loadedImages vector
        std::mutex queueMutex;  // For protecting the operation queue

        void ProcessImageQueue() {
            std::lock_guard<std::mutex> lock(queueMutex);
            while (!imageOperationQueue.empty()) {
                SaveOperation op = imageOperationQueue.front();
                imageOperationQueue.pop();

                switch (op.type) {
                case OperationType::SaveEntityImage:
                    SaveImage(op.entityID);
                    break;
                case OperationType::SaveImage:
                    SaveImage(op.component);
                    break;
                case OperationType::LoadImage:
                    // Handle load image operation if needed
                    break;
                default:
                    break;
                }
            }
        }

        FileType GetFileType(const std::string& extension) {
            std::string lowerExt = extension;
            // Convert to lowercase for case-insensitive comparison
            std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (lowerExt == "png")
                return FileType::PNG;
            if (lowerExt == "jpg" || lowerExt == "jpeg")
                return FileType::JPG;
            if (lowerExt == "bmp")
                return FileType::BMP;
            if (lowerExt == "tga")
                return FileType::TGA;
            if (lowerExt == "hdr")
                return FileType::HDR;
            return FileType::Unsupported;
        }

        std::string GetFileExtension(const std::string& filePath) {
            size_t pos = filePath.find_last_of('.');
            if (pos != std::string::npos) {
                return filePath.substr(pos + 1);
            }
            return ""; // No extension found
        }

        void HandleSave(const ImageComponent& imageComp) {
            if (imageComp.textureID == 0 && !imageComp.imageData) {
                std::cerr << "No valid image data to save for Entity ID " << imageComp.GetID() << "." << std::endl;
                return;
            }

            // Determine the image dimensions and data
            int width, height;
            std::vector<unsigned char> pixels;

            if (imageComp.imageData) {
                // Use the image data directly if available
                width = imageComp.width;
                height = imageComp.height;
                int dataSize = width * height * imageComp.channels;
                pixels.resize(dataSize);
                std::memcpy(pixels.data(), imageComp.imageData, dataSize);
            }
            else if (imageComp.textureID) {
                // Read from texture if no direct image data
                GLint texWidth, texHeight;
                glBindTexture(GL_TEXTURE_2D, imageComp.textureID);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

                width = texWidth;
                height = texHeight;

                pixels.resize(width * height * 4); // 4 bytes per pixel (RGBA)
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            }

            // Ensure directory exists
            std::filesystem::path path(imageComp.filePath);
            std::filesystem::path dir = path.parent_path();
            if (!dir.empty() && !std::filesystem::exists(dir)) {
                std::filesystem::create_directories(dir);
            }

            // Extract file extension and determine the save format
            std::string extension = GetFileExtension(imageComp.filePath);
            int channels = imageComp.imageData ? imageComp.channels : 4;

            switch (GetFileType(extension)) {
            case FileType::PNG:
                if (stbi_write_png(imageComp.filePath.c_str(), width, height, channels, pixels.data(), width * channels)) {
                    std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << imageComp.filePath << "."
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as PNG." << std::endl;
                }
                break;
            case FileType::JPG:
                if (stbi_write_jpg(imageComp.filePath.c_str(), width, height, channels, pixels.data(), 90)) {
                    std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << imageComp.filePath << "."
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as JPG." << std::endl;
                }
                break;
            case FileType::BMP:
                if (stbi_write_bmp(imageComp.filePath.c_str(), width, height, channels, pixels.data())) {
                    std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << imageComp.filePath << "."
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as BMP." << std::endl;
                }
                break;
            case FileType::TGA:
                if (stbi_write_tga(imageComp.filePath.c_str(), width, height, channels, pixels.data())) {
                    std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << imageComp.filePath << "."
                        << std::endl;
                }
                else {
                    std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as TGA." << std::endl;
                }
                break;
            case FileType::HDR:
                if (channels == 3 || channels == 4) {
                    // Convert 8-bit data to float for HDR
                    std::vector<float> hdrData(width * height * channels);
                    for (int i = 0; i < width * height * channels; ++i) {
                        hdrData[i] = pixels[i] / 255.0f;
                    }

                    if (stbi_write_hdr(imageComp.filePath.c_str(), width, height, channels, hdrData.data())) {
                        std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << imageComp.filePath << "."
                            << std::endl;
                    }
                    else {
                        std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as HDR." << std::endl;
                    }
                }
                else {
                    std::cerr << "HDR format requires 3 or 4 channels, but image has " << channels << " channels." << std::endl;
                }
                break;
            default:
                std::cerr << "Unsupported file extension for saving image: " << extension << std::endl;
                // Default to PNG if extension not recognized
                if (stbi_write_png((imageComp.filePath + ".png").c_str(), width, height, channels, pixels.data(), width * channels)) {
                    std::cout << "Image saved with .png extension at " << imageComp.filePath + ".png" << std::endl;
                }
                break;
            }
        }
    };

} // namespace ECS

#endif // IMAGESYSTEM_HPP