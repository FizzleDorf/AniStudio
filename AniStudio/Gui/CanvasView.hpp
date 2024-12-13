#ifndef CANVAS_VIEW_HPP
#define CANVAS_VIEW_HPP

#include "ECS.h"
#include "LayerManager.hpp"
#include <glm/glm.hpp>
#include <imgui.h>
#include <vector>

class CanvasView {
public:
    CanvasView(ECS::EntityManager &ecsManager);

    void Render(); // Render the entire canvas and UI
    void Update(); // Update logic (e.g., brush strokes)

private:
    // ECS and Layer Manager references
    ECS::EntityManager &ecsManager;
    LayerManager layerManager;

    // Canvas properties
    GLuint canvasFBO;     // Framebuffer Object
    GLuint canvasTexture; // Canvas texture
    glm::vec2 canvasSize; // Size of the canvas

    // Brush properties
    struct Brush {
        glm::vec4 color; // RGBA
        float size;      // Brush size
    } brush;

    // Private methods
    void RenderCanvas();        // Render the drawing canvas
    void RenderBrushSettings(); // UI for brush settings
    void RenderLayerManager();  // UI for layer manager
    void InitializeCanvas();    // Initialize the framebuffer and texture
};

#endif // CANVAS_VIEW_HPP
