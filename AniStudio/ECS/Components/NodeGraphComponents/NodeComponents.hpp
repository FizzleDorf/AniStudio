#pragma once
#include "Base/BaseComponent.hpp"
#include <glm/glm.hpp>
#include <imgui.h>

namespace ECS {

struct NodeComponent : public BaseComponent {
    NodeComponent() { compName = "NodeComponent"; }
    std::string name;
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{150.0f, 100.0f};
    ImU32 color = IM_COL32(60, 60, 60, 255);
    bool dragging = false;
    std::string state;
};

struct PinComponent : public BaseComponent {
    PinComponent() { compName = "PinComponent"; }
    enum class Type { Flow, Bool, Int, Float, String, Object, Function, Delegate };
    enum class Kind { Input, Output };

    std::string name;
    Type type = Type::Flow;
    Kind kind = Kind::Input;
    glm::vec2 position{0.0f, 0.0f};
    EntityID nodeEntity = 0;
};

struct LinkComponent : public BaseComponent {
    LinkComponent() { compName = "LinkComponent"; }
    EntityID startPin = 0;
    EntityID endPin = 0;
    ImU32 color = IM_COL32(255, 255, 255, 255);
};

} // namespace ECS