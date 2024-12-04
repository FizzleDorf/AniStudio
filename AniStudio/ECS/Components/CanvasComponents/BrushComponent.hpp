#ifndef BRUSH_COMPONENT_HPP
#define BRUSH_COMPONENT_HPP

#include "BaseComponent.hpp"
namespace ECS {
struct BrushComponent : public BaseComponent {
    float size = 5.0f;
    float color[3];

    BrushComponent() {
        color[0] = color[1] = color[2] = 0.0f; // Default black brush
    }
};
} // namespace ECS
#endif // BRUSH_COMPONENT_HPP
