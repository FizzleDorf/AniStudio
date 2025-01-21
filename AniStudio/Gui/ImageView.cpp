#include "ImageView.hpp"
#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <algorithm>
#include <stdexcept>
#include "pch.h"

using namespace ECS;

namespace GUI {

static float zoom = 1.0f;
static float offsetX, offsetY = 0;

void ImageView::Render() {
    ImGui::SetNextWindowSize(ImVec2(1024, 1024), ImGuiCond_FirstUseEver);
    ImGui::Begin("Image Viewer", nullptr); //, ImGuiWindowFlags_AlwaysAutoResize);

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

    ImGui::SameLine();

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

    RenderHistory();

    ImGui::Separator();

    if (ImGui::BeginChild("ImageViewerChild", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (imageComponent.imageData) {
            // Handle zooming with mouse wheel when child window is hovered
            if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
                SetZoom(zoom + ImGui::GetIO().MouseWheel * 0.1f);
            }

            // Calculate the window padding
            const ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 contentPos = ImVec2(windowPos.x + windowPadding.x, windowPos.y + windowPadding.y);

            // Calculate image size and position
            ImVec2 imageSize = ImVec2(imageComponent.width * zoom, imageComponent.height * zoom);
            ImVec2 imagePos = ImVec2(offsetX + windowPadding.x, offsetY + windowPadding.y);

            // Draw grid before the image
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            const float gridStep = 100.0f * zoom;

            // Get the visible content area
            const ImVec2 windowSize = ImGui::GetWindowSize();
            const ImVec2 contentMin = windowPos;
            const ImVec2 contentMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);

            // Calculate grid start positions
            float startX = contentMin.x - fmodf(ImGui::GetScrollX(), gridStep);
            float startY = contentMin.y - fmodf(ImGui::GetScrollY(), gridStep);

            // Draw vertical grid lines
            for (float x = startX; x < contentMax.x; x += gridStep) {
                draw_list->AddLine(ImVec2(x, contentMin.y), ImVec2(x, contentMax.y), IM_COL32(255, 255, 255, 50));
            }

            // Draw horizontal grid lines
            for (float y = startY; y < contentMax.y; y += gridStep) {
                draw_list->AddLine(ImVec2(contentMin.x, y), ImVec2(contentMax.x, y), IM_COL32(255, 255, 255, 50));
            }

            // Set cursor position and render image
            ImGui::SetCursorPos(imagePos);
            ImGui::Image((void *)(intptr_t)imageComponent.textureID, imageSize, ImVec2(0, 0), ImVec2(1, 1),
                         ImVec4(1, 1, 1, 1));

            // Handle panning only when the child window is hovered
            if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                offsetX += ImGui::GetIO().MouseDelta.x;
                offsetY += ImGui::GetIO().MouseDelta.y;
            }

            // Ensure the content size is set to accommodate the image
            ImGui::SetCursorPos(ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y));
        } else {
            ImGui::Text("No image loaded.");
        }
        ImGui::EndChild();
    }

    /*if (imageComponent.imageData) {
        ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(imageComponent.textureID)),
                     ImVec2(static_cast<float>(imageComponent.width), static_cast<float>(imageComponent.height)));
    } else {
        ImGui::Text("No image loaded.");
    }*/

    ImGui::End();
}

void ImageView::SetZoom(float newZoom) {
    // Limit zoom level to a reasonable range
    zoom = std::clamp(newZoom, 0.1f, 5.0f); // Limit zoom between 0.1 and 5.0
}

void ImageView::DrawGrid() {
    // Draw the grid on the canvas, adjusting for zoom and panning
    ImVec2 min_(offsetX, offsetY);
    ImVec2 max_(offsetX + imageComponent.width * zoom, offsetY + imageComponent.height * zoom);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    float step = 100.0f * zoom; // Grid step adjusts based on zoom
    for (float x = min_.x + step; x < max_.x; x += step) {
        draw_list->AddLine(ImVec2(x, min_.y), ImVec2(x, max_.y), IM_COL32(255, 255, 255, 50));
    }
    for (float y = min_.y + step; y < max_.y; y += step) {
        draw_list->AddLine(ImVec2(min_.x, y), ImVec2(max_.x, y), IM_COL32(255, 255, 255, 50));
    }
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
    ImGui::Checkbox("Show History", &showHistory);

    if (!loadedMedia.GetImages().empty() && showHistory) {
        // Navigation controls
        if (ImGui::Button("First")) {
            imgIndex = 0;
            imageComponent = loadedMedia.GetImage(imgIndex);
            CleanUpCurrentImage();
            CreateCurrentTexture();
        }
        ImGui::SameLine();
        if (ImGui::Button("Last")) {
            imgIndex = static_cast<int>(loadedMedia.GetImages().size() - 1);
            imageComponent = loadedMedia.GetImage(imgIndex);
            CleanUpCurrentImage();
            CreateCurrentTexture();
        }
        ImGui::SameLine();
        ImGui::Text("History Size: %zu", loadedMedia.GetImages().size());

        // Create scrollable history panel
        if (ImGui::BeginChild("HistoryPanel", ImVec2(0, 160), true, ImGuiWindowFlags_HorizontalScrollbar)) {
            const auto &images = loadedMedia.GetImages();

            for (size_t i = 0; i < images.size(); ++i) {
                ImGui::BeginGroup();
                const auto &image = images[i];

                if (image.textureID == 0) {
                    CreateTexture(i);
                }
                if (!image.imageData) {
                    mgr.DestroyEntity(image.GetID());
                    ImGui::EndGroup();
                    break;
                }

                if (static_cast<int>(i) == imgIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                }
                ImGui::Text("%zu: %s", i, image.fileName.c_str());
                if (static_cast<int>(i) == imgIndex) {
                    ImGui::PopStyleColor();
                }

                // Calculate image dimensions
                float aspectRatio = static_cast<float>(image.width) / static_cast<float>(image.height);
                ImVec2 maxSize(128.0f, 128.0f);
                ImVec2 imageSize;
                if (aspectRatio > 1.0f) {
                    imageSize = ImVec2(maxSize.x, maxSize.x / aspectRatio);
                } else {
                    imageSize = ImVec2(maxSize.y * aspectRatio, maxSize.y);
                }

                // Make image clickable
                if (ImGui::ImageButton(("##img" + std::to_string(i)).c_str(),
                                       reinterpret_cast<void *>(static_cast<intptr_t>(image.textureID)), imageSize)) {
                    imgIndex = static_cast<int>(i);
                    imageComponent = loadedMedia.GetImage(imgIndex);
                    CleanUpCurrentImage();
                    CreateCurrentTexture();
                }

                ImGui::EndGroup();
                ImGui::SameLine();
            }
            ImGui::NewLine();
        }
        ImGui::EndChild();
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
    event.type = ANI::EventType::LoadImageEvent;
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