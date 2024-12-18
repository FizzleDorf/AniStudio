#pragma once

#include "BaseComponent.hpp"
#include <GL/glew.h>
#include <string>

namespace ECS {
struct ImageComponent : public BaseComponent {
    compName = "Image Component";
    std::string fileName = "image.png"; // Default file name
    std::string filePath = "";          // Full path to the image
    unsigned char *imageData = nullptr; // Pointer to image data
    int width = 0;                      // Image width
    int height = 0;                     // Image height
    int channels = 0;                   // Number of color channels
    GLuint textureID = 0;               // OpenGL texture ID

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }

    ImageComponent &operator=(const ImageComponent &other) {
        if (this != &other) { // Self-assignment check
            fileName = other.fileName;
            filePath = other.filePath;
            imageData = other.imageData;
            width = other.width;
            height = other.height;
            channels = other.channels;
            textureID = other.textureID;
        }
        return *this;
    }
};

struct InputImageComponent : public ImageComponent {};
struct OutputImageComponent : public ImageComponent {};
} // namespace ECS
