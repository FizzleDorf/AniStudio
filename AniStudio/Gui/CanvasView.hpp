#ifndef CANVAS_VIEW_HPP
#define CANVAS_VIEW_HPP

#include "BaseView.hpp"
#include "LayerManager.hpp"
#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>

struct Point {
    float x, y;
    ImVec4 color;
};

class CanvasView : public BaseView {
public:
    CanvasView();
    void Init() { InitializeCanvas(); }
    void Render();                            // Render the entire canvas and UI
    void Update(const float deltaT) override; // Update logic (e.g., brush strokes)

private:
    // ECS and Layer Manager references
    LayerManager layerManager;
    std::vector<Point> points;
    // Canvas properties
    GLuint canvasFBO;     // Framebuffer Object
    GLuint canvasTexture; // Canvas texture
    glm::vec2 canvasSize; // Size of the canvas

    // Brush properties
    struct Brush {
        glm::vec4 color; // RGBA
        float size;      // Brush size
    } brush;

    // State
    glm::vec2 lastMousePos; // Last mouse position
    bool isDrawing = false; // Whether the user is drawing
    int currentLayerIndex;  // Index of the currently active layer

    // Private methods
    void RenderCanvas();        // Render the drawing canvas
    void RenderBrushSettings(); // UI for brush settings
    void RenderLayerManager();  // UI for layer manager
    void InitializeCanvas();    // Initialize the framebuffer and texture
    void DrawOnLayer();         // Draw on the current layer
};

#endif // CANVAS_VIEW_HPP
