#include "BaseSystem.hpp"
#include "ECS.h"

class RenderSystem : public ECS::BaseSystem {
public:
    void Render() {
        for (auto &entity : m_Entities) {
            auto &drawable = entity.GetComponent<DrawableComponent>();
            auto &transform = entity.GetComponent<TransformComponent>();
            auto &color = entity.GetComponent<ColorComponent>();

            // Bind shaders, set uniforms (e.g., color, transform)
            DrawShape(drawable, transform, color);
        }
    }

    void Update() {
        // Process mouse clicks and key presses
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glm::vec2 mousePos = GetMousePosition();
            UpdateBrush(mousePos);
        }
    }
};
