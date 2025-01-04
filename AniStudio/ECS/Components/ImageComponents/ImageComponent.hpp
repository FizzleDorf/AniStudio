#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include <GL/glew.h>
#include <string>
#include <stb_image.h>

namespace ECS {
struct ImageComponent : public BaseComponent {
    std::string fileName = "Anistudio";                     // Default file name
    std::string filePath = filePaths.defaultProjectPath; // Full path to the image
    unsigned char *imageData = nullptr;                  // Pointer to image data
    int width = 0;                                       // Image width
    int height = 0;                                      // Image height
    int channels = 0;                                    // Number of color channels
    GLuint textureID = 0;                                // OpenGL texture ID

    ~ImageComponent() {
        if (imageData) {
            stbi_image_free(imageData);
        }
    }
    
    ImageComponent &operator=(const ImageComponent &other) {
        if (this != &other) {
            fileName = other.fileName;
            filePath = other.filePath;
            width = other.width;
            height = other.height;
            channels = other.channels;
        }
        return *this;
    }
};

struct InputImageComponent : public ImageComponent {};
} // namespace ECS
