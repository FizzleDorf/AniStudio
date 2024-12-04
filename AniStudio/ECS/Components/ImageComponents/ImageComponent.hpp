#pragma once

#include "BaseComponent.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <filesystem>
#include <string>
#include <GL/glew.h>

#define SD_API

namespace ECS {
struct ImageComponent : public ECS::BaseComponent {
    std::string fileName = "image.png"; // Default file name
    std::string filePath = "";          // Full path to the image
    unsigned char *imageData = nullptr; // Pointer to image data
    int width = 0;                      // Image width
    int height = 0;                     // Image height
    int channels = 0;                   // Number of color channels
    GLuint textureID = 0;

    ImageComponent &operator=(const ImageComponent &other) {
        if (this != &other) {
            fileName = other.fileName;
            filePath = other.filePath;
            imageData = other.imageData;
            width = other.width;
            height = other.height;
            channels = other.channels;
            textureID = other.channels;
        }
        return *this;
    }

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }

    bool loadImageFromPath() {
        if (imageData) {
            stbi_image_free(imageData);
        }

        imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
        if (!imageData) {
            return false; // Loading failed
        }

        // Update file name from the path
        fileName = std::filesystem::path(filePath).filename().string();
        return true;
    }

    void SetImageData(unsigned char *data, int imgWidth, int imgHeight, int imgChannels) {
        if (imageData) {
            stbi_image_free(imageData);
        }

        imageData = data;
        width = imgWidth;
        height = imgHeight;
        channels = imgChannels;
    }

    bool saveImage(const std::string &outputPath) {
        if (!imageData || width == 0 || height == 0 || channels == 0) {
            return false; // Cannot save an invalid image
        }

        // Extract file extension
        std::string extension = std::filesystem::path(outputPath).extension().string();

        // Save based on file extension
        bool success = false;
        if (extension == ".png") {
            success = stbi_write_png(outputPath.c_str(), width, height, channels, imageData, width * channels);
        } else if (extension == ".jpg" || extension == ".jpeg") {
            success = stbi_write_jpg(outputPath.c_str(), width, height, channels, imageData, 100); // Quality: 100
        } else if (extension == ".bmp") {
            success = stbi_write_bmp(outputPath.c_str(), width, height, channels, imageData);
        } else if (extension == ".tga") {
            success = stbi_write_tga(outputPath.c_str(), width, height, channels, imageData);
        }

        if (success) {
            filePath = outputPath; // Update file path
            fileName = std::filesystem::path(outputPath).filename().string();
        }

        return success;
    }

    bool saveImageAs(const std::string &outputPath) { return saveImage(outputPath); }
};

struct InputImageComponent : public ECS::ImageComponent {};
struct OutputImageComponent : public ECS::ImageComponent {};
} // namespace ECS
