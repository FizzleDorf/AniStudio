#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

    struct PromptComponent : public BaseComponent {
        PromptComponent() {
            compName = "Prompt";

            // Define the component schema
            schema = {
                {"title", "Prompt Settings"},
                {"type", "object"},
                {"propertyOrder", {"posPrompt", "negPrompt"}},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 52.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"posPrompt", {
                        {"type", "string"},
                        {"title", "Positive"},
                        {"ui:widget", "textarea"},
                        {"ui:flags", {ImGuiInputTextFlags_AllowTabInput}},
                        {"ui:options", {
                            {"rows", 8}
                        }}
                    }},
                    {"negPrompt", {
                        {"type", "string"},
                        {"title", "Negative"},
                        {"ui:widget", "textarea"},
                        {"ui:flags", {ImGuiInputTextFlags_AllowTabInput}},
                        {"ui:options", {
                            {"rows", 8}
                        }}
                    }}
                }}
            };
        }

        std::string posPrompt = "";
        std::string negPrompt = "";

        // Override the GetPropertyMap method to map component properties to UI schema
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["posPrompt"] = &posPrompt;
            properties["negPrompt"] = &negPrompt;
            return properties;
        }

        PromptComponent& operator=(const PromptComponent& other) {
            if (this != &other) {
                posPrompt = other.posPrompt;
                negPrompt = other.negPrompt;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"posPrompt", posPrompt},
                {"negPrompt", negPrompt}
            };
            return j;
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

            if (componentData.contains("posPrompt")) {
                posPrompt = componentData["posPrompt"].get<std::string>();
            }

            if (componentData.contains("negPrompt")) {
                negPrompt = componentData["negPrompt"].get<std::string>();
            }
        }
    };

} // namespace ECS