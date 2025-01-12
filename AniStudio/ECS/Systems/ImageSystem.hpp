#ifndef IMAGESYSTEM_HPP
#define IMAGESYSTEM_HPP

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "LoadedMedia.hpp"
#include <GL/glew.h>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace ECS {

class ImageSystem : public BaseSystem {
public:
    ImageSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) { sysName = "ImageSystem"; }
    ~ImageSystem() = default;

    void Update(const float deltaT) {}

    void AddImage(const ImageComponent &image) { loadedImages.push_back(image); }
    void AddImage(const EntityID entity) { loadedImages.push_back(mgr.GetComponent<ImageComponent>(entity)); }

    void RemoveImage(const size_t index) {
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        loadedImages.erase(loadedImages.begin() + index);
    }

    ImageComponent &GetImage(const size_t index) {
        if (index >= loadedImages.size()) {
            throw std::out_of_range("Image index out of range");
        }
        return loadedImages[index];
    }

    std::vector<ImageComponent> &GetImages() { return loadedImages; }

    void SaveImage(const EntityID entityID) { 
        const auto &imageComp = mgr.GetComponent<ImageComponent>(entityID); 
        HandleSave(imageComp);
    }
    void SaveImage(const ImageComponent &imageComp) { HandleSave(imageComp); }

private:
    std::vector<ImageComponent> loadedImages;
    std::queue<ImageComponent> imageQueue;

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

    void HandleSave(const ImageComponent &imageComp) {
        if (imageComp.textureID == 0) {
            std::cerr << "No valid texture to save for Entity ID " << imageComp.GetID() << "." << std::endl;
            return;
        }

        // Get the texture size
        GLint width, height;
        glBindTexture(GL_TEXTURE_2D, imageComp.textureID);
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
                std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << filePath << "."
                          << std::endl;
            } else {
                std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as PNG." << std::endl;
            }
            break;
        case FileType::JPG:
            if (stbi_write_jpg(filePath.c_str(), width, height, 4, pixels.data(), 90)) {
                std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << filePath << "."
                          << std::endl;
            } else {
                std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as JPG." << std::endl;
            }
            break;
        case FileType::BMP:
            if (stbi_write_bmp(filePath.c_str(), width, height, 4, pixels.data())) {
                std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << filePath << "."
                          << std::endl;
            } else {
                std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as BMP." << std::endl;
            }
            break;
        case FileType::TGA:
            if (stbi_write_tga(filePath.c_str(), width, height, 4, pixels.data())) {
                std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << filePath << "."
                          << std::endl;
            } else {
                std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as TGA." << std::endl;
            }
            break;
        case FileType::HDR:
            if (stbi_write_hdr(filePath.c_str(), width, height, 4, reinterpret_cast<float *>(pixels.data()))) {
                std::cout << "Image saved for Entity ID " << imageComp.GetID() << " at " << filePath << "."
                          << std::endl;
            } else {
                std::cerr << "Failed to save image for Entity ID " << imageComp.GetID() << " as HDR." << std::endl;
            }
            break;
        default:
            std::cerr << "Unsupported file extension for saving image: " << extension << std::endl;
            break;
        }
    }
};

} // namespace ECS

#endif // IMAGESYSTEM_HPP
