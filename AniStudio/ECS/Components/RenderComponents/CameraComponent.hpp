#pragma once
#include "BaseComponent.hpp"
#include <glm/glm.hpp>

namespace ECS {

struct CameraComponent : public BaseComponent {
    glm::mat4 viewMatrix = glm::mat4(1.0f);       // View matrix
    glm::mat4 projectionMatrix = glm::mat4(1.0f); // Projection matrix
};

} // namespace ECS
