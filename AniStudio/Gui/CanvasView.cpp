#include "CanvasView.hpp"
#include <GL/glew.h>
#include <iostream>

template <typename T>
T lerp(const T &a, const T &b, float t) {
    return a + t * (b - a);
}

// Constructor
CanvasView::CanvasView()
    : canvasSize(800.0f, 600.0f), brush({glm::vec4(1.0f), 5.0f}), currentLayerIndex(0), canvas_size(1024.0f, 1024.0f) {
    layerManager.SetCanvasSize(canvasSize.x, canvasSize.y);
}

// Initialize the canvas framebuffer and texture

void CanvasView::Init() {
    // Ensure OpenGL context is active
    if (!glGetString(GL_VERSION)) {
        std::cerr << "OpenGL context not active!" << std::endl;
        return;
    }

    // Log canvas size
    std::cout << "Canvas size: " << canvas_size.x << "x" << canvas_size.y << std::endl;

    // Generate framebuffer
    glGenFramebuffers(1, &canvasFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, canvasFBO);

    // Create texture for the canvas
    glGenTextures(1, &canvasTexture);
    glBindTexture(GL_TEXTURE_2D, canvasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, canvas_size.x, canvas_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Attach texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvasTexture, 0);

    // Check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
    }

    // Unbind framebuffer
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
    // Begin the graph container
    ImGui::Begin("Canvas View");

    if (ImGui::Button("Save Canvas")) {
        SaveCanvasToFile("canvas_output.png");
    }

    static ImVec2 scrolling(0.0f, 0.0f); // Initial scrolling offset
    static float zoom = 1.0f;            // Initial zoom level
    static const float zoomMin = 0.1f;
    static const float zoomMax = 3.0f;

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

    // Draw grid lines (optional)
    float grid_step = 50.0f * zoom;
    for (float x = fmodf(scrolling.x * zoom, grid_step); x < canvas_sz.x; x += grid_step)
        draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y),
                           IM_COL32(200, 200, 200, 40));
    for (float y = fmodf(scrolling.y * zoom, grid_step); y < canvas_sz.y; y += grid_step)
        draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y),
                           IM_COL32(200, 200, 200, 40));

    // Render the canvas at (0, 0) in graph space
    ImVec2 canvas_pos = graph_origin; // Canvas is always at (0, 0) in graph space

    // Draw canvas background
    draw_list->AddRectFilled(canvas_pos,
                             ImVec2(canvas_pos.x + canvas_size.x * zoom, canvas_pos.y + canvas_size.y * zoom),
                             IM_COL32(255, 255, 255, 255));

    // Handle painting on the canvas
    if (ImGui::IsMouseHoveringRect(canvas_pos,
                                   ImVec2(canvas_pos.x + canvas_size.x * zoom, canvas_pos.y + canvas_size.y * zoom)) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 local_pos = ImVec2((mouse_pos.x - canvas_pos.x) / zoom, (mouse_pos.y - canvas_pos.y) / zoom);

        // Draw a circle at the clicked position
        draw_list->AddCircleFilled(
            ImVec2(canvas_pos.x + local_pos.x * zoom, canvas_pos.y + local_pos.y * zoom), brush.size * zoom,
            IM_COL32(brush.color.r * 255, brush.color.g * 255, brush.color.b * 255, brush.color.a * 255));
    } else if (ImGui::IsMouseHoveringRect(
                   canvas_pos, ImVec2(canvas_pos.x + canvas_size.x * zoom, canvas_pos.y + canvas_size.y * zoom))) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 local_pos = ImVec2((mouse_pos.x - canvas_pos.x) / zoom, (mouse_pos.y - canvas_pos.y) / zoom);

        draw_list->AddCircle(
            ImVec2(canvas_pos.x + local_pos.x * zoom, canvas_pos.y + local_pos.y * zoom), brush.size * zoom,
            IM_COL32(brush.color.r * 255, brush.color.g * 255, brush.color.b * 255, brush.color.a * 255));
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

void CanvasView::SaveCanvasToFile(const std::string &filename) {
    // Save the current framebuffer
    GLint prevFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFramebuffer);

    // Bind the canvas framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, canvasFBO);

    // Allocate memory for pixel data
    const int width = static_cast<int>(canvas_size.x);
    const int height = static_cast<int>(canvas_size.y);
    std::vector<unsigned char> pixels(width * height * 4); // RGBA format

    // Read pixels from the framebuffer
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Restore the previous framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer);

    // Flip the image vertically (OpenGL origin is bottom-left, but most images expect top-left)
    for (int y = 0; y < height / 2; ++y) {
        int topIndex = y * width * 4;
        int bottomIndex = (height - 1 - y) * width * 4;
        for (int x = 0; x < width * 4; ++x) {
            std::swap(pixels[topIndex + x], pixels[bottomIndex + x]);
        }
    }

    // Save the pixels to a PNG file
    if (!stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4)) {
        std::cerr << "Failed to save canvas to file: " << filename << std::endl;
    } else {
        std::cout << "Canvas saved to file: " << filename << std::endl;
    }
}
