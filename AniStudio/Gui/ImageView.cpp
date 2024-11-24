#include "ImageView.hpp"
#include "ImGuiFileDialog.h" // For file dialogs
#include <imgui.h>
#include <iostream>
#include <stb_image_write.h>
#include <stdexcept>

namespace ECS {

ImageView::ImageView() : textureID(0) {}

void ImageView::SetImageComponent(const ImageComponent &component) {
    imageComponent = component; // Assign the component object
    CreateTexture();            // Generate the OpenGL texture
}

void ImageView::Render() {
    ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);
    ImGui::Begin("Image Viewer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Display image details if loaded
    if (imageComponent.imageData) {
        ImGui::Text("File: %s", imageComponent.fileName.c_str());
        ImGui::Text("Dimensions: %dx%d", imageComponent.width, imageComponent.height);
        ImGui::Separator();
    }

    // Load Image Button
    if (ImGui::Button("Load Image")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image", ".png,.jpg,.jpeg,.bmp,.tga", config);
    }

    // Handle file dialog for loading images
    if (ImGuiFileDialog::Instance()->Display("LoadImageDialog")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                LoadImage(selectedPath);
                std::cout << "Image loaded: " << selectedPath << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error loading image: " << e.what() << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Save Image Button
    if (imageComponent.imageData && ImGui::Button("Save Image")) {
        ImGui::OpenPopup("Confirm Save");
    }

    // Confirmation popup for saving
    if (ImGui::BeginPopupModal("Confirm Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to overwrite the existing file?");
        ImGui::Separator();

        if (ImGui::Button("Yes")) {
            try {
                SaveImage(imageComponent.filePath);
                std::cout << "Image saved: " << imageComponent.filePath << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error saving image: " << e.what() << std::endl;
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Save As Button
    if (imageComponent.imageData && ImGui::Button("Save Image As")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        ImGuiFileDialog::Instance()->OpenDialog("SaveImageAsDialog", "Save Image As", ".png,.jpg,.jpeg,.bmp,.tga",
                                                config);
    }

    // Handle file dialog for saving as
    if (ImGuiFileDialog::Instance()->Display("SaveImageAsDialog")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                SaveImage(savePath);
                std::cout << "Image saved as: " << savePath << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error saving image as: " << e.what() << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Display loaded image
    if (imageComponent.imageData) {
        ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(textureID)),
                     ImVec2(static_cast<float>(imageComponent.width), static_cast<float>(imageComponent.height)));
    } else {
        ImGui::Text("No image loaded.");
    }

    ImGui::End();
}

void ImageView::LoadImage(const std::string &filePath) {
    imageComponent.filePath = filePath;
    if (imageComponent.loadImageFromPath()) {
        CreateTexture();
    } else {
        throw std::runtime_error("Failed to load image data.");
    }
}

void ImageView::SaveImage(const std::string &filePath) {
    if (imageComponent.saveImage(filePath)) {
        std::cout << "Image successfully saved to: " << filePath << std::endl;
    } else {
        throw std::runtime_error("Failed to save image.");
    }
}

void ImageView::CreateTexture() {
    if (textureID) {
        glDeleteTextures(1, &textureID);
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageComponent.width, imageComponent.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 imageComponent.imageData);

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
