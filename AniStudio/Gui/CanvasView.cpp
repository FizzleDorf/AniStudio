#include "CanvasView.hpp"
#include "ImageUtils.hpp" // Use the Util class for image handling

namespace GUI {

CanvasView::CanvasView(ECS::EntityManager &manager)
    : BaseView(manager), zoom(1.0f), offsetX(0.0f), offsetY(0.0f), currentHistoryIndex(0) {
    InitializeCanvas(1024, 1024);
}

CanvasView::~CanvasView() {}

void CanvasView::InitializeCanvas(int width, int height) {
    // Initialize a white canvas (empty image)
    currentImage = cv::Mat::ones(height, width, CV_8UC3) * 255; // White canvas
    originalImage = currentImage.clone();                       // Keep the original canvas for undo/redo
}

void CanvasView::SetZoom(float newZoom) {
    // Limit zoom level to a reasonable range
    zoom = std::clamp(newZoom, 0.1f, 5.0f); // Limit zoom between 0.1 and 5.0
}

void CanvasView::ApplyBrushStroke(const ImVec2 &position, float size) {
    // Scale the size of the brush based on zoom level
    float scaledSize = size * zoom;

    // Apply the brush stroke to the current image (example: red brush)
    cv::circle(currentImage, cv::Point(position.x, position.y), static_cast<int>(scaledSize), cv::Scalar(255, 0, 0),
               -1);

    // Save the current state to history for undo/redo
    if (currentHistoryIndex < strokeHistory.size() - 1) {
        strokeHistory.erase(strokeHistory.begin() + currentHistoryIndex + 1, strokeHistory.end());
    }
    strokeHistory.push_back(currentImage.clone());
    currentHistoryIndex = strokeHistory.size() - 1;

    CreateTexture(); // Re-create the texture with the new image
}

void CanvasView::Undo() {
    if (currentHistoryIndex > 0) {
        currentHistoryIndex--;
        currentImage = strokeHistory[currentHistoryIndex];
        CreateTexture(); // Re-create texture from the new image
    }
}

void CanvasView::Redo() {
    if (currentHistoryIndex < strokeHistory.size() - 1) {
        currentHistoryIndex++;
        currentImage = strokeHistory[currentHistoryIndex];
        CreateTexture(); // Re-create texture from the new image
    }
}

void CanvasView::Render() {
    ImGui::Begin("CanvasView");

    // Handle zooming with mouse wheel
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        SetZoom(zoom + ImGui::GetIO().MouseWheel * 0.1f); // Zoom in/out based on wheel scroll
    }

    // Handle left mouse click to draw
    if (ImGui::IsMouseDown(0)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        // Convert mouse position to canvas space by applying offsets and zoom
        ImVec2 canvasPos = ImVec2((mousePos.x - offsetX) / zoom, (mousePos.y - offsetY) / zoom);
        ApplyBrushStroke(canvasPos, 10.0f); // Adjust brush size as needed
    }

    // Draw the grid first
    DrawGrid();

    // Then render the current image (canvas) on top of the grid
    ImVec2 imageSize = ImVec2(currentImage.cols * zoom, currentImage.rows * zoom);
    ImVec2 imagePos = ImVec2(offsetX, offsetY); // Apply panning offsets

    // Apply zoom and position to the image
    ImGui::SetCursorPos(imagePos); // Set the starting position for the image
    ImGui::Image((void *)(intptr_t)textureID, imageSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1));

    // Handle mouse dragging for panning the canvas
    if (ImGui::IsMouseDragging(0)) {
        offsetX += ImGui::GetIO().MouseDelta.x;
        offsetY += ImGui::GetIO().MouseDelta.y;
    }

    ImGui::End();
}

void CanvasView::DrawGrid() {
    // Draw the grid on the canvas, adjusting for zoom and panning
    ImVec2 min(offsetX, offsetY);
    ImVec2 max(offsetX + currentImage.cols * zoom, offsetY + currentImage.rows * zoom);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    float step = 100.0f * zoom; // Grid step adjusts based on zoom
    for (float x = min.x + step; x < max.x; x += step) {
        draw_list->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), IM_COL32(255, 255, 255, 50));
    }
    for (float y = min.y + step; y < max.y; y += step) {
        draw_list->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), IM_COL32(255, 255, 255, 50));
    }
}

bool CanvasView::CreateTexture() {
    // Create OpenGL texture from the current canvas image
    if (!textureID) {
        glGenTextures(1, &textureID);
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, currentImage.cols, currentImage.rows, 0, GL_BGR, GL_UNSIGNED_BYTE,
                 currentImage.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}
} // namespace GUI