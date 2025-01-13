#include "ImageView.hpp"
#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>
#include <vector>

using namespace ECS;

namespace GUI {

void ImageView::Render() {
    ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);
    ImGui::Begin("Image Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (imageComponent.imageData) {
        ImGui::Text("File: %s", imageComponent.fileName.c_str());
        ImGui::Text("Dimensions: %dx%d", imageComponent.width, imageComponent.height);
        ImGui::Separator();
    }

    RenderSelector();

    if (ImGui::Button("Load Image")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image", ".png,.jpg,.jpeg,.bmp,.tga", config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            imageComponent.filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            imageComponent.fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
            LoadImage();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();

    if (imageComponent.imageData && ImGui::Button("Save Image")) {
        SaveImage(imageComponent.filePath);
    }

    if (imageComponent.imageData && ImGui::Button("Save Image As")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveImageAsDialog", "Save Image As", ".png,.jpg,.jpeg,.bmp,.tga",
                                                config);
    }

    if (ImGuiFileDialog::Instance()->Display("SaveImageAsDialog")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
            SaveImage(savePath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (imageComponent.imageData) {
        ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(imageComponent.textureID)),
                     ImVec2(static_cast<float>(imageComponent.width), static_cast<float>(imageComponent.height)));
    } else {
        ImGui::Text("No image loaded.");
    }

    RenderHistory();
    ImGui::End();
}

void ImageView::RenderSelector() {
    if (ImGui::InputInt("Current Image", &imgIndex)) {
        if (loadedMedia.GetImages().empty()) {
            imgIndex = 0;
            return;
        }
        const int size = loadedMedia.GetImages().size();
        if (size == 1) {
            imgIndex = 0;
        } else {
            if (imgIndex < 0) {
                imgIndex = size - 1;
            }
            imgIndex = (imgIndex % size + size) % size;
        } 
        imageComponent = loadedMedia.GetImage(imgIndex);
        CleanUpCurrentImage();
        CreateCurrentTexture();     
    }
}

void ImageView::RenderHistory() {
    if (ImGui::Checkbox("Show History", &showHistory) && showHistory) {
         
    }
    if (!loadedMedia.GetImages().empty() && showHistory) {
        const auto &images = loadedMedia.GetImages();
        for (size_t i = 0; i < images.size(); ++i) {
            const auto &image = images[i];
            if (image.textureID == 0) {
                CreateTexture(i);
            }
            if (!image.imageData) {
                mgr.DestroyEntity(image.GetID());
                break;
            }
            ImGui::Text("Image %zu: %s", i, image.fileName.c_str());
            ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(image.textureID)),
                         ImVec2(static_cast<float>(image.width / 4), static_cast<float>(image.height / 4)));
        }
    }
}

void ImageView::LoadImage() {
    CleanUpCurrentImage();
    CreateCurrentTexture();

    EntityID newEntity = mgr.AddNewEntity();
    mgr.AddComponent<ImageComponent>(newEntity);
    mgr.GetComponent<ImageComponent>(newEntity) = imageComponent;
    ANI::Event event;
    event.entityID = newEntity;
    event.type = ANI::EventType::LoadImage;
    loadedMedia.AddImage(mgr.GetComponent<ImageComponent>(newEntity));
    // ANI::Events::Ref().QueueEvent(event);
    imgIndex = static_cast<int>(loadedMedia.GetImages().size() - 1);
}

void ImageView::SaveImage(const std::string &filePath) {
    ANI::Event event;
    // event.entityID = newEntity;
    // event.type = ANI::EventType::SaveImage;
    // ANI::Events::Ref().QueueEvent(event);
    if (imageComponent.imageData) {
        if (!stbi_write_png(filePath.c_str(), imageComponent.width, imageComponent.height, imageComponent.channels,
                            imageComponent.imageData, imageComponent.width * imageComponent.channels)) {
            throw std::runtime_error("Failed to save image to: " + filePath);
        }
    }
}

void ImageView::CreateTexture(const int index) {
    ImageComponent &image = loadedMedia.GetImage(index);

    if (image.imageData) {
        stbi_image_free(image.imageData);
        image.imageData = nullptr;
    }

    if (image.textureID) {
        glDeleteTextures(1, &image.textureID);
        image.textureID = 0;
    }

    image.imageData = stbi_load(image.filePath.c_str(), &image.width, &image.height, &image.channels, 0);
    if (!image.imageData) {
        throw std::runtime_error("Failed to load image: " + image.filePath);
    }
    glGenTextures(1, &image.textureID);
    glBindTexture(GL_TEXTURE_2D, image.textureID);
    GLenum format = (image.channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageView::CreateCurrentTexture() {
    imageComponent.imageData = stbi_load(imageComponent.filePath.c_str(), &imageComponent.width, &imageComponent.height,
                                         &imageComponent.channels, 0);
    if (!imageComponent.imageData) {
        throw std::runtime_error("Failed to load image: " + imageComponent.filePath);
    }
    glGenTextures(1, &imageComponent.textureID);
    glBindTexture(GL_TEXTURE_2D, imageComponent.textureID);
    GLenum format = (imageComponent.channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, imageComponent.width, imageComponent.height, 0, format, GL_UNSIGNED_BYTE,
                 imageComponent.imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ImageView::CleanUpCurrentImage() {
    if (imageComponent.imageData) {
        stbi_image_free(imageComponent.imageData);
        imageComponent.imageData = nullptr;
    }
    if (imageComponent.textureID) {
        glDeleteTextures(1, &imageComponent.textureID);
        imageComponent.textureID = 0;
    }
}

ImageView::~ImageView() { CleanUpCurrentImage(); }

} // namespace ECS