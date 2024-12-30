#include "CanvasView.hpp"
#include <GL/glew.h>
#include <iostream>

// Constructor
CanvasView::CanvasView() : canvasSize(800.0f, 600.0f), brush({glm::vec4(1.0f), 5.0f}), currentLayerIndex(0) {
    layerManager.SetCanvasSize(canvasSize.x, canvasSize.y);
    InitializeCanvas();
}

// Initialize the canvas framebuffer and texture
void CanvasView::InitializeCanvas() {
    /*glGenFramebuffers(1, &canvasFBO);
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
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

    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos(); // Top-left corner of the canvas
    ImVec2 canvas_sz = ImVec2(canvasSize.x, canvasSize.y); // Fixed canvas size
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y); // Bottom-right corner

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Draw canvas background
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255)); // Dark gray background
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));    // White border

    // Check if the mouse is inside the canvas and draw
    if (ImGui::IsMouseHoveringRect(canvas_p0, canvas_p1) && ImGui::IsMouseDown(0)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();

        // Clamp the brush position to the canvas boundaries
        mouse_pos.x = std::clamp(mouse_pos.x, canvas_p0.x, canvas_p1.x - 1);
        mouse_pos.y = std::clamp(mouse_pos.y, canvas_p0.y, canvas_p1.y - 1);

        points.push_back({mouse_pos.x, mouse_pos.y, ImVec4(brush.color.r, brush.color.g, brush.color.b, brush.color.a)});
    }

    // Draw points
    for (const auto &point : points) {
        draw_list->AddCircleFilled(ImVec2(point.x, point.y), brush.size, IM_COL32(point.color.x * 255, point.color.y * 255, point.color.z * 255, point.color.w * 255));
    }

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
    if (ImGui::Button("Remove Layer") && layerManager.GetLayerCount() > 0) {
        layerManager.RemoveLayer(currentLayerIndex);
    }

    ImGui::Text("Number of Layers: %d", static_cast<int>(layerManager.GetLayerCount()));

    // Active layer selector
    if (layerManager.GetLayerCount() > 0) {
        ImGui::SliderInt("Active Layer", &currentLayerIndex, 0, layerManager.GetLayerCount() - 1);
    }

    ImGui::End();
}

// Update method (for brush strokes)
void CanvasView::Update(const float deltaT) {
    //if (ImGui::IsMouseDown(0)) { // Left mouse button is down
    //    glm::vec2 mousePos = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y) -
    //                         glm::vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);

    //    if (mousePos.x < 0 || mousePos.y < 0 || mousePos.x >= canvasSize.x || mousePos.y >= canvasSize.y) {
    //        std::cerr << "Error: Mouse position out of bounds!" << std::endl;
    //        return; // Prevent out-of-bounds drawing
    //    }

    //    if (!isDrawing) {
    //        isDrawing = true;
    //        lastMousePos = mousePos;
    //    }

    //    // Interpolate points and draw
    //    float distance = glm::distance(lastMousePos, mousePos);
    //    if (distance > brush.size / 2.0f) {
    //        int steps = static_cast<int>(distance / (brush.size / 2.0f));
    //        glm::vec2 stepVector = (mousePos - lastMousePos) / static_cast<float>(steps);

    //        for (int i = 0; i <= steps; ++i) {
    //            glm::vec2 interpolatedPoint = lastMousePos + stepVector * static_cast<float>(i);
    //            // Draw to the current layer
    //            DrawOnLayer();
    //        }
    //    }

    //    lastMousePos = mousePos;
    //} else {
    //    isDrawing = false;
    //}
}

// Draw on the current layer
void CanvasView::DrawOnLayer() {
    glBindFramebuffer(GL_FRAMEBUFFER, canvasFBO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, canvasSize.x, canvasSize.y);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, canvasSize.x, canvasSize.y, 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(brush.color.r, brush.color.g, brush.color.b, brush.color.a);
    glBegin(GL_QUADS);
    float halfSize = brush.size / 2.0f;

    glVertex2f(lastMousePos.x - halfSize, lastMousePos.y - halfSize);
    glVertex2f(lastMousePos.x + halfSize, lastMousePos.y - halfSize);
    glVertex2f(lastMousePos.x + halfSize, lastMousePos.y + halfSize);
    glVertex2f(lastMousePos.x - halfSize, lastMousePos.y + halfSize);
    glEnd();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
}
