#include "pch.h"
#include "BaseComponent.hpp"

namespace ECS {
struct TransformComponent : public BaseComponent {
    glm::vec2 position;
    float rotation;
    glm::vec2 scale;
};

struct ColorComponent : public BaseComponent {
    glm::vec4 color; // RGBA
};

struct BrushComponent : public BaseComponent {
    float size;
    int type;       // 0 = circle, 1 = square, etc.
    float pressure; // For dynamic effects
};

struct DrawableComponent : public BaseComponent {
    std::vector<glm::vec2> vertices;
    bool isDirty; // Flag to indicate changes
};
} // namespace ECS