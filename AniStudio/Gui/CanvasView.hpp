#pragma once
#include "Base/BaseView.hpp"
#include <imgui.h>
#include <opencv2/opencv.hpp>
#include <vector>
namespace GUI {
class CanvasView {
public:
    CanvasView();
    ~CanvasView();

    // Initialize a white canvas of size 1024x1024
    void InitializeCanvas(int width = 1024, int height = 1024);

    // Set zoom factor (for zooming in/out)
    void SetZoom(float zoom);

    // Apply brush stroke on canvas (e.g., at a given position)
    void ApplyBrushStroke(const ImVec2 &position, float size);

    // Undo the last stroke
    void Undo();

    // Redo the last undone stroke
    void Redo();

    // Render the canvas, grid, and brush
    void Render();

private:
    cv::Mat currentImage;   // The current canvas image
    cv::Mat originalImage;  // To store the original canvas for undo/redo
    float zoom;             // Zoom factor
    float offsetX, offsetY; // For panning the canvas

    std::vector<cv::Mat> strokeHistory; // History of strokes for undo/redo
    int currentHistoryIndex;            // Current index in the stroke history

    GLuint textureID; // OpenGL texture ID for rendering the canvas

    void DrawGrid();      // Draw grid based on zoom factor
    bool CreateTexture(); // Create OpenGL texture from the current canvas
};
} // namespace UI