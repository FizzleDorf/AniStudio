#include "CanvasView.hpp"
#include <GL/glew.h>
#include <iostream>

template <typename T>
T lerp(const T &a, const T &b, float t) {
    return a + t * (b - a);
}

// Constructor
CanvasView::CanvasView() : canvasSize(800.0f, 600.0f), brush({glm::vec4(1.0f), 5.0f}), currentLayerIndex(0) {
    layerManager.SetCanvasSize(canvasSize.x, canvasSize.y);
    InitializeCanvas();
}

// Initialize the canvas framebuffer and texture
void CanvasView::InitializeCanvas() {}

// Main render method
void CanvasView::Render() {
    RenderCanvas();
    RenderBrushSettings();
    RenderLayerManager();
}

// Render the drawing canvas
void CanvasView::RenderCanvas() {
    // Begin the graph editor
    ImGui::Begin("Canvas Editor");

    static bool isActive = false;        // Indicates if the graph is active
    static ImVec2 scrolling(0.0f, 0.0f); // Initial scrolling offset
    static float zoom = 1.0f;            // Initial zoom level
    static const float zoomMin = 0.1f;
    static const float zoomMax = 3.0f;

    // Check if the graph is active
    isActive = ImGui::IsWindowFocused();

    // Get the available space for the graph
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();                                  // Top-left corner
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();                               // Available space
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y); // Bottom-right corner

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Draw background and border
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(30, 30, 30, 255)); // Dark gray background
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));    // White border

    // Handle zooming with the mouse wheel
    ImGuiIO &io = ImGui::GetIO();
    if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
        float zoomDelta = io.MouseWheel * 0.1f;
        zoom = std::clamp(zoom + zoomDelta, zoomMin, zoomMax);
    }

    // Handle panning with mouse drag
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
        ImVec2 delta = io.MouseDelta;
        scrolling.x += delta.x / zoom;
        scrolling.y += delta.y / zoom;
    }

    // Transform for zoom and pan
    ImVec2 graph_origin = ImVec2(canvas_p0.x + scrolling.x * zoom, canvas_p0.y + scrolling.y * zoom);

    // Draw grid lines
    float grid_step = 50.0f * zoom;
    for (float x = fmodf(scrolling.x * zoom, grid_step); x < canvas_sz.x; x += grid_step)
        draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y),
                           IM_COL32(200, 200, 200, 40));
    for (float y = fmodf(scrolling.y * zoom, grid_step); y < canvas_sz.y; y += grid_step)
        draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y),
                           IM_COL32(200, 200, 200, 40));

    // Draw existing paint strokes
    static std::vector<ImVec2> points;
    for (const auto &point : points) {
        ImVec2 transformed_point = ImVec2(graph_origin.x + point.x * zoom, graph_origin.y + point.y * zoom);
        draw_list->AddCircleFilled(transformed_point, brush.size * zoom,
            IM_COL32(static_cast<int>(brush.color.r * 255.0f), static_cast<int>(brush.color.g * 255.0f),
                     static_cast<int>(brush.color.b * 255.0f), static_cast<int>(brush.color.a * 255.0f)));
    }

    // Handle painting with interpolation for smoother strokes
    if (isActive) {
        if (ImGui::IsMouseHoveringRect(canvas_p0, canvas_p1) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 local_pos = ImVec2((mouse_pos.x - graph_origin.x) / zoom, (mouse_pos.y - graph_origin.y) / zoom);

            // Interpolate points to add smoothness to the stroke
            if (!points.empty()) {
                // Get the last point and interpolate to the new one
                ImVec2 last_point = points.back();
                InterpolatePoints(last_point, local_pos, brush.size * 0.1f,
                                  points); // Step size proportional to brush size
            }

            points.push_back(local_pos); // Add the current point
        }
    } else {
        ImGui::Text("Painting inactive: Pin the graph and ensure it is active.");
    }


    // End the graph editor
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

void CanvasView::Update(const float deltaT) {}

void CanvasView::InterpolatePoints(const ImVec2 &p0, const ImVec2 &p1, float step, std::vector<ImVec2> &points) {
    float distance = glm::distance(glm::vec2(p0.x, p0.y), glm::vec2(p1.x, p1.y));
    int numPoints = static_cast<int>(distance / step);

    for (int i = 0; i <= numPoints; ++i) {
        float t = i / float(numPoints);
        float x = lerp(p0.x, p1.x, t);
        float y = lerp(p0.y, p1.y, t);
        points.push_back(ImVec2(x, y));
    }
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

