#include "ImageView.hpp"
#include <iostream>
#include <stb_image_write.h> // For saving images
#include <stdexcept>

namespace ECS {

ImageView::ImageView() : imageComponent(nullptr), textureID(0) {}

void ImageView::SetImageComponent(ImageComponent *component) {
    imageComponent = component;
    if (imageComponent && imageComponent->imageData) {
        CreateTexture();
    }
}

void ImageView::Render() {
    ImGui::Begin("Image Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (imageComponent && imageComponent->imageData && textureID) {
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(textureID)),
                     ImVec2(static_cast<float>(imageComponent->width), static_cast<float>(imageComponent->height)));
    }
    ImGui::End();
}


void ImageView::LoadImage(const std::string &filePath) {
    if (!imageComponent) {
        throw std::runtime_error("ImageComponent is not set.");
    }
    imageComponent->loadImageFromPath(filePath);
    if (imageComponent->imageData) {
        CreateTexture();
    } else {
        throw std::runtime_error("Failed to load image data.");
    }
}

void ImageView::SaveImage(const std::string &filePath) {
    if (!imageComponent || !imageComponent->imageData) {
        throw std::runtime_error("No image data to save.");
    }
    stbi_write_png(filePath.c_str(), imageComponent->width, imageComponent->height, 4, imageComponent->imageData,
                   imageComponent->width * 4);
}

void ImageView::CreateTexture() {
    if (!imageComponent || !imageComponent->imageData) {
        throw std::runtime_error("Image data is not loaded.");
    }

    if (textureID) {
        glDeleteTextures(1, &textureID);
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageComponent->width, imageComponent->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 imageComponent->imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

ImageView::~ImageView() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }
}

} // namespace ECS
