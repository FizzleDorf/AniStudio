#include "ImageView.hpp"
#include "pch.h"
#include <GL/glew.h>
#include <ImGuiFileDialog.h>
#include <algorithm>
#include <imgui.h>
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>

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

    if (ImGui::Button("Load Image(s)")) {
        IGFD::FileDialogConfig config;
        config.path = ".";
        // Enable multiple selection in the config
        config.countSelectionMax = 0; // 0 means infinite selections
        ImGuiFileDialog::Instance()->OpenDialog("LoadImageDialog", "Choose Image(s)", filters, config);
    }

    if (ImGuiFileDialog::Instance()->Display("LoadImageDialog", 32, ImVec2(700, 400))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            // Get multiple selections
            std::map<std::string, std::string> selection = ImGuiFileDialog::Instance()->GetSelection();

            std::vector<std::string> filePaths;
            for (const auto &[fileName, filePath] : selection) {
                filePaths.push_back(filePath);
            }

            LoadImages(filePaths);
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
        