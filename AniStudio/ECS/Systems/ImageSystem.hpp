#ifndef IMAGESYSTEM_HPP
#define IMAGESYSTEM_HPP

#include "BaseSystem.hpp"
#include "ImageComponent.hpp"
#include "EntityManager.hpp"
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
        for (const auto &entityID : images) {
            if (mgr.HasComponent<ImageComponent>(entityID)) {
                auto &imageComp = mgr.GetComponent<ImageComponent>(entityID);
                DeleteTexture(imageComp.textureID);
            }
        }
    }

    void Start() override {}

    // Recieved a new image from generation or canvas
    void NewImage(const EntityID entityID) {
        if (mgr.HasComponent<ImageComponent>(entityID)) {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entityID);
            GLuint textureID = LoadTextureFromFile(imageComp.filePath);
            if (textureID != 0) {
                imageComp.textureID = textureID;
                std::cout << "Image loaded for Entity ID " << entityID << "." << std::endl;
            } else {
                std::cerr << "Failed to load image for Entity ID " << entityID << "." << std::endl;
            }
        } else {
            std::cerr << "Entity ID " << entityID << " has no Image Component" << std::endl;
            return;
        }
    }

    // Find the ImageComponent and load its image based on the file path
    void LoadImage(const EntityID entityID) {  
        if (mgr.HasComponent<ImageComponent>(entityID)) {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entityID);
            GLuint textureID = LoadTextureFromFile(imageComp.filePath);
            if (textureID != 0) {
                imageComp.textureID = textureID;
                std::cout << "Image loaded for Entity ID " << entityID << "." << std::endl;
            } else {
                std::cerr << "Failed to load image for Entity ID " << entityID << "." << std::endl;
            }
        } else {
            std::cerr << "Entity ID " << entityID << " has no Image Component" << std::endl;
            return;
        }
    }
    // Find the ImageComponent and save the current image based on the file path + extension
    void SaveImage(const EntityID entityID) {
        if (mgr.HasComponent<ImageComponent>(entityID)) {
            auto &imageComp = mgr.GetComponent<ImageComponent>(entityID);

            // Get texture details
            GLuint textureID = imageComp.textureID;
            if (textureID == 0) {
                std::cerr << "No valid texture to save for Entity ID " << entityID << "." << std::endl;
                return;
            }

            // Get the texture size
            GLint width, height;
            glBindTexture(GL_TEXTURE_2D, textureID);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

            // Read the texture pixels back from OpenGL
            std::vector<unsigned char> pixels(width * height * 4); // 4 bytes per pixel (RGBA)
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

            // Extract file extension and determine the save format
            std::string filePath = imageComp.filePath;
            std::string extension = GetFileExtension(filePath);
            
            switch (GetFileType(extension)) {
            case FileType::PNG:
                if (stbi_write_png(filePath.c_str(), width, height, 4, pixels.data(), width * 4)) {
                    std::cout << "Image saved for Entity ID " << entityID << " at " << filePath << "." << std::endl;
                } else {
                    std::cerr << "Failed to save image for Entity ID " << entityID << " as PNG." << std::endl;
                }
                break;
            case FileType::JPG:
                if (stbi_write_jpg(filePath.c_str(), width, height, 4, pixels.data(), 90)) {
                    std::cout << "Image saved for Entity ID " << entityID << " at " << filePath << "." << std::endl;
                } else {
                    std::cerr << "Failed to save image for Entity ID " << entityID << " as JPG." << std::endl;
                }
                break;
            case FileType::BMP:
                if (stbi_write_bmp(filePath.c_str(), width, height, 4, pixels.data())) {
                    std::cout << "Image saved for Entity ID " << entityID << " at " << filePath << "." << std::endl;
                } else {
                    std::cerr << "Failed to save image for Entity ID " << entityID << " as BMP." << std::endl;
                }
                break;
            case FileType::TGA:
                if (stbi_write_tga(filePath.c_str(), width, height, 4, pixels.data())) {
                    std::cout << "Image saved for Entity ID " << entityID << " at " << filePath << "." << std::endl;
                } else {
                    std::cerr << "Failed to save image for Entity ID " << entityID << " as TGA." << std::endl;
                }
                break;
            case FileType::HDR:
                if (stbi_write_hdr(filePath.c_str(), width, height, 4, reinterpret_cast<float *>(pixels.data()))) {
                    std::cout << "Image saved for Entity ID " << entityID << " at " << filePath << "." << std::endl;
                } else {
                    std::cerr << "Failed to save image for Entity ID " << entityID << " as HDR." << std::endl;
                }
                break;
            default:
                std::cerr << "Unsupported file extension for saving image: " << extension << std::endl;
                break;
            }
        } else {
            std::cerr << "Entity ID " << entityID << " has no Image Component" << std::endl;
        }
    }

private:
    std::vector<EntityID> images;

    enum class FileType { PNG, JPG, BMP, TGA, HDR, Unsupported };
    
    FileType GetFileType(const std::string &extension) {
        if (extension == "png")
            return FileType::PNG;
        if (extension == "jpg" || extension == "jpeg")
            return FileType::JPG;
        if (extension == "bmp")
            return FileType::BMP;
        if (extension == "tga")
            return FileType::TGA;
        if (extension == "hdr")
            return FileType::HDR;
        return FileType::Unsupported;
    }

    std::string GetFileExtension(const std::string &filePath) {
        size_t pos = filePath.find_last_of('.');
        if (pos != std::string::npos) {
            return filePath.substr(pos + 1);
        }
        return ""; // No extension found
    }

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
