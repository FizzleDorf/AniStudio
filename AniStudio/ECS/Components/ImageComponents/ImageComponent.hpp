#pragma once

#include "BaseComponent.hpp"
#include "stb_image.h"
#include <string>

namespace ECS {
struct ImageIOComponent : public ECS::BaseComponent {
    unsigned char *imageData = nullptr;
    int width = 0;
    int height = 0;
    int channels = 0;

    ~InputImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    };

    bool loadImageFromPath(const std::string &path) {
        if (imageData) {
            stbi_image_free(imageData);
        }

        imageData = stbi_load(path.c_str(), &width, &height, &channels, 0);
        return imageData != nullptr;
    };
};
}
