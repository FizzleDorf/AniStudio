#pragma once

#include "BaseComponent.hpp"
#include <string>
#include "stb_image.h"
#include "stb_image_write.h"
#define SD_API
namespace ECS {
struct ImageComponent : public ECS::BaseComponent {
    unsigned char *imageData = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;

    ImageComponent &operator=(const ImageComponent &other) {
        if (this != &other) {
            imageData = other.imageData;
            width = other.width;
            height = other.height;
            channels = other.channels;
        }
        return *this;
    }

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }
    
    bool loadImageFromPath(const std::string &path) {
        if (imageData) {
            stbi_image_free(imageData);
        }

        imageData = stbi_load(path.c_str(), &width, &height, &channels, 0);
        return imageData != nullptr;
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
};

struct InputImageComponent : public ECS::ImageComponent {};
struct OutputImageComponent : public ECS::ImageComponent {};
}

