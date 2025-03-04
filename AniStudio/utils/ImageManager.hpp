#pragma once

#include "pch.h"
#include "Types.hpp"
#include "ImageComponent.hpp"
#include <GL/glew.h>
#include <unordered_map>
#include <string>
#include <memory>

namespace ECS {

    class ImageManager {
    public:
        ImageManager() = default;
        ~ImageManager() {
            // Clean up all images
            for (auto& pair : imageCache) {
                if (pair.second.textureID != 0) {
                    glDeleteTextures(1, &pair.second.textureID);
                }
            }
        }

        // Load image and return the texture ID
        GLuint LoadImage(const std::string& filePath) {
            // Check if image is already loaded
            auto it = imageCache.find(filePath);
            if (it != imageCache.end()) {
                // Increment reference count
                it->second.refCount++;
                return it->second.textureID;
            }

            // Load the image
            int width, height, channels;
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);

            if (!data) {
                std::cerr << "Failed to load image: " << filePath << std::endl;
                return 0;
            }

            // Create OpenGL texture
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Upload image data
            GLenum format = channels == 4 ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Free image data
            stbi_image_free(data);

            // Store in cache
            ImageInfo info;
            info.textureID = textureID;
            info.width = width;
            info.height = height;
            info.channels = channels;
            info.refCount = 1;

            imageCache[filePath] = info;

            return textureID;
        }

        // Release image (decrement reference count and delete if no longer used)
        void ReleaseImage(const std::string& filePath) {
            auto it = imageCache.find(filePath);
            if (it != imageCache.end()) {
                it->second.refCount--;
                if (it->second.refCount <= 0) {
                    // Delete texture if no longer referenced
                    glDeleteTextures(1, &it->second.textureID);
                    imageCache.erase(it);
                }
            }
        }

        // Get image info
        bool GetImageInfo(const std::string& filePath, int& width, int& height, int& channels) {
            auto it = imageCache.find(filePath);
            if (it != imageCache.end()) {
                width = it->second.width;
                height = it->second.height;
                channels = it->second.channels;
                return true;
            }
            return false;
        }

        // Apply image to an ImageComponent
        void ApplyImageToComponent(ImageComponent& component) {
            if (!component.filePath.empty()) {
                // Load the image
                component.textureID = LoadImage(component.filePath);

                // Update component information
                if (component.textureID != 0) {
                    auto it = imageCache.find(component.filePath);
                    if (it != imageCache.end()) {
                        component.width = it->second.width;
                        component.height = it->second.height;
                        component.channels = it->second.channels;
                    }
                }
            }
        }

    private:
        struct ImageInfo {
            GLuint textureID = 0;
            int width = 0;
            int height = 0;
            int channels = 0;
            int refCount = 0;
        };

        std::unordered_map<std::string, ImageInfo> imageCache;
    };

}  // namespace ECS