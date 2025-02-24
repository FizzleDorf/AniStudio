#pragma once
#include "BaseComponent.hpp"
#include "ComponentMetadata.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace ECS {

// Represents an active pin connection on a node
struct PinConnection {
    std::string name;
    ComponentProperty::Type type;
    bool isConnected = false;
    EntityID connectedEntityId = 0;
    size_t connectedPinIndex = 0;
};

struct NodeComponent : public BaseComponent {
    std::string name;
    glm::vec2 position;
    glm::vec2 size;
    std::vector<PinConnection> inputs;
    std::vector<PinConnection> outputs;
    bool isDirty = true;

    ComponentMetadata GetMetadata() const override {
        ComponentMetadata metadata;
        metadata.name = name;
        metadata.category = "Node";
        metadata.description = "Node component for visual programming";
        return metadata;
    }

    void RenderProperties(EntityManager &mgr, EntityID entity) override {
        ImGui::Text("Name", &name);
        ImGui::DragFloat2("Position", &position[0]);
        ImGui::DragFloat2("Size", &size[0]);
    }
};

struct LinkComponent : public BaseComponent {
    EntityID startNode;
    size_t startPinIndex;
    EntityID endNode;
    size_t endPinIndex;
    float thickness = 1.0f;

    ComponentMetadata GetMetadata() const override {
        ComponentMetadata metadata;
        metadata.name = "Link";
        metadata.category = "Node";
        metadata.description = "Connection between node pins";
        return metadata;
    }

    void RenderProperties(EntityManager &mgr, EntityID entity) override {
        ImGui::DragFloat("Thickness", &thickness, 0.1f, 0.1f, 10.0f);
    }
};

} // namespace ECS