#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

    struct ClipSkipComponent : public ECS::BaseComponent {
        ClipSkipComponent() {
            compName = "ClipSkip";

            // Define the component schema
            schema = {
                {"title", "Clip Skip Settings"},
                {"type", "object"},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 64.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"clipSkip", {
                        {"type", "number"},
                        {"title", "Clip Skip"},
                        {"ui:widget", "drag_float"},
                        {"ui:options", {
                            {"min", 0.0f},
                            {"max", 12.0f},
                            {"speed", 0.1f}
                        }}
                    }}
                }}
            };
        }

        float clipSkip = 2.0f;

        // Override the GetPropertyMap method
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["clipSkip"] = &clipSkip;
            return properties;
        }

        ClipSkipComponent& operator=(const ClipSkipComponent& other) {
            if (this != &other) {
                clipSkip = other.clipSkip;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"clipSkip", clipSkip}
                     }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);

            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("clipSkip"))
                clipSkip = componentData["clipSkip"];
        }
    };

} // namespace ECS