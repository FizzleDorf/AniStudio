#pragma once
#include "BaseComponent.hpp"
#include "BaseView.hpp"
#include "imgui.h"

namespace CustomPlugin {

// Custom Component
struct CustomComponent : public ECS::BaseComponent {
    CustomComponent() { compName = "CustomComponent"; }

    float value = 0.0f;
    std::string text = "Custom Text";

    nlohmann::json Serialize() const override {
        auto j = BaseComponent::Serialize();
        j["value"] = value;
        j["text"] = text;
        return j;
    }

    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("value"))
            value = j["value"];
        if (j.contains("text"))
            text = j["text"];
    }
};

// Custom View
class CustomView : public BaseView {
public:
    void Render() override {
        ImGui::Begin("Custom View");

        // Get all entities with CustomComponent
        for (const auto &entity : ECS::mgr.GetAllEntities()) {
            if (ECS::mgr.HasComponent<CustomComponent>(entity)) {
                auto &comp = ECS::mgr.GetComponent<CustomComponent>(entity);

                std::string label = "Entity " + std::to_string(entity);
                if (ImGui::TreeNode(label.c_str())) {
                    ImGui::SliderFloat("Value", &comp.value, 0.0f, 1.0f);

                    // Buffer for text input
                    static char buffer[256];
                    strcpy_s(buffer, comp.text.c_str());
                    if (ImGui::InputText("Text", buffer, sizeof(buffer))) {
                        comp.text = buffer;
                    }

                    ImGui::TreePop();
                }
            }
        }

        if (ImGui::Button("Add Entity with CustomComponent")) {
            auto entity = ECS::mgr.AddNewEntity();
            ECS::mgr.AddComponent<CustomComponent>(entity);
        }

        ImGui::End();
    }
};

// Custom System
class CustomSystem : public ECS::BaseSystem {
public:
    CustomSystem() {
        sysName = "CustomSystem";
        AddComponentSignature<CustomComponent>();
    }

    void Update(const float deltaT) override {
        for (const auto &entity : entities) {
            auto &comp = ECS::mgr.GetComponent<CustomComponent>(entity);
            comp.value = std::fmod(comp.value + deltaT, 1.0f);
        }
    }
};

} // namespace CustomPlugin