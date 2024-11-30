#ifndef IMAGESYSTEM_HPP
#define IMAGESYSTEM_HPP

#include "../../ECS/Base/BaseSystem.hpp"
#include <GL/glew.h>
#include <iostream>
#include <stb_image.h>
#include <string>
#include <unordered_map>

namespace ECS {

class ImageSystem : public BaseSystem {
public:
    ImageSystem() = default;
    ~ImageSystem() {
        // Cleanup textures when ImageSystem is destroyed
        for (auto &[entityID, data] : images) {
            DeleteTexture(data.textureID);
        }
    }

    void Start() override {
        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW!" << std::endl;
            return;
        }
        std::cout << "GLEW initialized successfully." << std::endl;
    }

    void LoadImage(EntityID entityID) {
        // Find the entity and load its image based on the file path
        auto it = images.find(entityID);
        if (it == images.end()) {
            std::cerr << "Entity ID " << entityID << " not found in ImageSystem." << std::endl;
            return;
        }

        GLuint textureID = LoadTextureFromFile(it->second.filePath);
        if (textureID != 0) {
            it->second.textureID = textureID;
            std::cout << "Image loaded for Entity ID " << entityID << "." << std::endl;
        } else {
            std::cerr << "Failed to load image for Entity ID " << entityID << "." << std::endl;
        }
    }

    void SaveImage(EntityID entityID) {
        auto it = images.find(entityID);
        if (it != images.end()) {
            std::cout << "Saving image for Entity ID " << entityID << " at " << it->second.filePath << "." << std::endl;
            // Implement save logic if needed
        } else {
            std::cerr << "Entity ID " << entityID << " not found in ImageSystem." << std::endl;
        }
    }

    GLuint GetTextureID(EntityID entityID) const {
        auto it = images.find(entityID);
        return (it != images.end()) ? it->second.textureID : 0;
    }

private:
    struct ImageData {
        GLuint textureID = 0;
        std::string filePath;
    };

    std::unordered_map<EntityID, ImageData> images;

    GLuint LoadTextureFromFile(const std::string &filePath) {
        int width, height, channels;
        unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!data) {
            std::cerr << "Failed to load image from file: " << filePath << std::endl;
            return 0;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        return textureID;
    }

    void DeleteTexture(GLuint textureID) { glDeleteTextures(1, &textureID); }
};

} // namespace ECS

#endif // IMAGESYSTEM_HPP
