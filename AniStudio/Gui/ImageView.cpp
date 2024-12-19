#include "ImageView.hpp"
#include <iostream>

namespace ECS {

ImageView::ImageView() : imgIndex(0) {
    // Initialize the EntityManager and add a system or components if necessary
    // Assuming that ImageComponent and EntityManager exist
    mgr.RegisterSystem<ImageSystem>();
    EntityID entity = mgr.AddNewEntity();
    mgr.AddComponent<ImageComponent>(entity);
    imageComponent = mgr.GetComponent<ImageComponent>(entity);
    images.push_back(entity);
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
        config.path = "."; // Starting directory for file dialog
        ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image", ".png,.jpg,.jpeg,.bmp,.tga", config);
    }

    // Handle file dialog for loading images
    if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            imageComponent.filePath = ImGuiFileDialog::Instance()->GetFilePathName();
            imageComponent.fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
            try {
                LoadImage(); // Load the selected image
            } catch (const std::exception &e) {
                std::cerr << "Error loading image: " << e.what() << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();

    // Save Image Button
    if (imageComponent.imageData && ImGui::Button("Save Image")) {
        ImGui::OpenPopup("Confirm Save");
    }

    // Save Image As Button
    if (imageComponent.imageData && ImGui::Button("Save Image As")) {
        IGFD::FileDialogConfig config;
        config.path = "."; // Default directory for save as dialog
        ImGuiFileDialog::Instance()->OpenDialog("SaveImageAsDialog", "Save Image As", ".png,.jpg,.jpeg,.bmp,.tga",
                                                config);
    }

    // Handle file dialog for saving as
    if (ImGuiFileDialog::Instance()->Display("SaveImageAsDialog")) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string savePath = ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                SaveImage(savePath); // Save the current image to the new file path
                std::cout << "Image saved as: " << savePath << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error saving image as: " << e.what() << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Image navigation input
    if (ImGui::InputInt("Current Image", &imgIndex)) {
        imgIndex = std::clamp(imgIndex, 0, static_cast<int>(images.size() - 1)); // Ensure the index is valid

        CleanUpCurrentImage(); // Cleanup before switching

        // Fetch the new image component
        if (mgr.HasComponent<ImageComponent>(images[imgIndex])) {
            imageComponent = mgr.GetComponent<ImageComponent>(images[imgIndex]);
            if (imageComponent.imageData) {
                CreateTexture(); // Create texture for the new image
            } else {
                std::cerr << "Warning: Image data for the selected component is null." << std::endl;
            }
        }
    }

    // Display the loaded image
    if (imageComponent.imageData) {
        ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(imageComponent.textureID)),
                     ImVec2(static_cast<float>(imageComponent.width), static_cast<float>(imageComponent.height)));
    } else {
        ImGui::Text("No image loaded.");
    }

    ImGui::End();
}

void ImageView::LoadImage() {
    // Free any existing image data and textures
    CleanUpCurrentImage();

    // Load the new image using stb_image
    imageComponent.imageData = stbi_load(imageComponent.filePath.c_str(), &imageComponent.width, &imageComponent.height, &imageComponent.channels, 0);
    if (!imageComponent.imageData) {
        throw std::runtime_error("Failed to load image: " + imageComponent.filePath);
    }

    // Create texture after loading the image
    CreateTexture();

    // Add a new entity for this image and save it
    EntityID newEntity = mgr.AddNewEntity();
    images.push_back(newEntity);
    mgr.AddComponent<ImageComponent>(newEntity);
    mgr.GetComponent<ImageComponent>(newEntity) = imageComponent;
}

void ImageView::SaveImage(const std::string &filePath) {
    if (imageComponent.imageData) {
        // Save the image using stb_image_write or your preferred method
        if (!stbi_write_png(filePath.c_str(), imageComponent.width, imageComponent.height, imageComponent.channels,
                            imageComponent.imageData, 0)) {
            throw std::runtime_error("Failed to save image to: " + filePath);
        }
    }
}

void ImageView::CreateTexture() {
    if (imageComponent.textureID) {
        glDeleteTextures(1, &imageComponent.textureID); // Delete the old texture
    }

    if (!imageComponent.imageData || imageComponent.width <= 0 || imageComponent.height <= 0) {
        throw std::runtime_error("Invalid image data or dimensions.");
    }

    glGenTextures(1, &imageComponent.textureID);
    glBindTexture(GL_TEXTURE_2D, imageComponent.textureID);
    GLenum format = (imageComponent.channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, imageComponent.width, imageComponent.height, 0, format, GL_UNSIGNED_BYTE,
                 imageComponent.imageData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}

void ImageView::CleanUpCurrentImage() {
    // Free the image data and delete the texture for the currently loaded image
    if (imageComponent.imageData) {
        stbi_image_free(imageComponent.imageData);
        imageComponent.imageData = nullptr;
    }
    if (imageComponent.textureID) {
        glDeleteTextures(1, &imageComponent.textureID);
        imageComponent.textureID = 0;
    }
}

ImageView::~ImageView() {
    CleanUpCurrentImage(); // Cleanup any allocated resources
}

} // namespace ECS
