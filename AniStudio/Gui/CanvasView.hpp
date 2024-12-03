#ifndef CANVAS_VIEW_HPP
#define CANVAS_VIEW_HPP

#include "ECS.h"
#include "LayerManager.hpp"
#include <imgui.h>

class CanvasView {
public:
    CanvasView(int canvasWidth, int canvasHeight);

    void Render();

private:
    EntityID canvasEntity;
    CanvasComponent *canvas;
    BrushComponent *brush;
    LayerManager layerManager;

    void RenderCanvas();
    void RenderBrushSettings();
    void RenderLayerManager();
};

#endif // CANVAS_VIEW_HPP
