#include "CanvasView.hpp"
#include <GL/glew.h>
#include <iostream>

// Constructor
CanvasView::CanvasView(ECS::EntityManager &ecsManager)
    : ecsManager(ecsManager), canvasSize(800.0f, 600.0f), brush({glm::vec4(1.0f), 5.0f}) {
    layerManager.SetCanvasSize(canvasSize.x, canvasSize.y);
    // InitializeCanvas();
}

// Initialize the canvas framebuffer and texture
void CanvasView::InitializeCanvas() {
    glGenFramebuffers(1, &canvasFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, canvasFBO);

    glGenTextures(1, &canvasTexture);
    glBindTexture(GL_TEXTURE_2D, canvasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvasSize.x, canvasSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvasTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Canvas framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Main render method
void CanvasView::Render() {
    RenderCanvas();
    RenderBrushSettings();
    RenderLayerManager();
}

// Render the drawing canvas
void CanvasView::RenderCanvas() {
    ImGui::Begin("Canvas");

    // Render the framebuffer as a texture
    ImGui::Image((void *)(intptr_t)canvasTexture, ImVec2(canvasSize.x, canvasSize.y));

    ImGui::End();
}

// UI for brush settings
void CanvasView::RenderBrushSettings() {
    ImGui::Begin("Brush Settings");
    ImGui::ColorEdit4("Brush Color", &brush.color.r);
    ImGui::SliderFloat("Brush Size", &brush.size, 1.0f, 50.0f);
    ImGui::End();
}

// UI for layer manager
void CanvasView::RenderLayerManager() {
    ImGui::Begin("Layers");

    if (ImGui::Button("Add Layer")) {
        layerManager.AddLayer();
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Layer")) {
        // layerManager.RemoveLayer();
    }

    ImGui::Text("Number of Layers: %d", static_cast<int>(layerManager.GetLayerCount()));

    ImGui::End();
}

// Update method (for brush strokes)
void CanvasView::Update() {
    
}
