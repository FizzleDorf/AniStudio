#ifndef CANVAS_COMPONENT_HPP
#define CANVAS_COMPONENT_HPP

#include "BaseComponent.hpp"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
namespace ECS {
class CanvasComponent : public BaseComponent {
public:
    CanvasComponent() : framebuffer(0), texture(0), quadVAO(0), quadVBO(0), brushSize(10.0f) {
        brushColor[0] = brushColor[1] = brushColor[2] = 0.0f; // Default black brush
    }

    ~CanvasComponent() {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texture);
        glDeleteBuffers(1, &quadVBO);
        glDeleteVertexArrays(1, &quadVAO);
    }

    void SetBrush(float size, float r, float g, float b) {
        brushSize = size;
        brushColor[0] = r;
        brushColor[1] = g;
        brushColor[2] = b;
    }

    /*void DrawAt(int x, int y) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glPointSize(brushSize);
        glColor3f(brushColor[0], brushColor[1], brushColor[2]);
        glBegin(GL_POINTS);
        glVertex2f((float)x / width * 2 - 1, (float)y / height * 2 - 1);
        glEnd();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderCanvas() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderImGuiCanvas() {
        ImGui::Begin("Canvas");
        ImGui::Image((void *)(intptr_t)texture, ImVec2(width, height));
        ImGui::End();
    }

    void SetHW(const int newWidth, const int newHeight) {
        width = newWidth;
        height = newHeight;
    }*/

private:
    int width, height;
    GLuint framebuffer, texture;
    GLuint quadVAO, quadVBO;
    float brushSize;
    float brushColor[3];
};
} // namespace ECS
#endif // CANVAS_COMPONENT_HPP
