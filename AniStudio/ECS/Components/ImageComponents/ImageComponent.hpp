#pragma once

#include "BaseComponent.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <string>

namespace ECS {
struct ImageComponent : public ECS::BaseComponent {
    unsigned char *imageData = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;

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
}
