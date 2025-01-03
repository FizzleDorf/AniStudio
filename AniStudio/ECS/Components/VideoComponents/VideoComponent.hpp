#pragma once

#include "BaseComponent.hpp"
#include <GL/glew.h>
#include <stb_image.h>
#include <string>

namespace ECS {
struct VideoComponent : public BaseComponent {
    std::string fileName = "<none>"; // Default file name
    std::string filePath = "";          // Full path to the image
    unsigned char *imageData = nullptr; // Pointer to image data
    int width = 0;                      // Image width
    int height = 0;                     // Image height
    int channels = 0;                   // Number of color channels
    GLuint textureID = 0;               // OpenGL texture ID

    ~VideoComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }

    VideoComponent &operator=(const VideoComponent &other) {
        if (this != &other) { // Self-assignment check
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            channels = other.channels;
        }
        return *this;
    }
};

struct InputVideoComponent : public ImageComponent {};
struct OutputVideoComponent : public ImageComponent {};
} // namespace ECS
