#include "CanvasView.hpp"

CanvasView::CanvasView(int canvasWidth, int canvasHeight)
    : canvas(canvasWidth, canvasHeight), layerManager(canvasWidth, canvasHeight) {
    layerManager.AddLayer();
}

void CanvasView::Render() {
    RenderCanvas();
    RenderBrushSettings();
    RenderLayerManager();
}

void CanvasView::RenderCanvas() {
    canvas.SetBrush(brush.size, brush.color[0], brush.color[1], brush.color[2]);
    canvas.RenderImGuiCanvas();
}

void CanvasView::RenderBrushSettings() {
    ImGui::Begin("Brush Settings");
    ImGui::SliderFloat("Brush Size", &brush.size, 1.0f, 50.0f);
    ImGui::ColorEdit3("Brush Color", brush.color);
    ImGui::End();
}

void CanvasView::RenderLayerManager() {
    ImGui::Begin("Layers");
    if (ImGui::Button("Add Layer")) {
        layerManager.AddLayer();
    }
    ImGui::Text("Number of Layers: %d", static_cast<int>(layerManager.GetLayerCount()));
    ImGui::End();
}
