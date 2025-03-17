#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
    struct LatentComponent : public ECS::BaseComponent {
        LatentComponent() {
            compName = "Latent";

            // Define the component schema
            schema = {
                {"title", "Latent Settings"},
                {"type", "object"},
                {"propertyOrder", {"latentWidth", "latentHeight", "batchSize"}},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 64.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"latentWidth", {
                        {"type", "integer"},
                        {"title", "Width"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 8},
                            {"step_fast", 32}
                        }}
                    }},
                    {"latentHeight", {
                        {"type", "integer"},
                        {"title", "Height"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 8},
                            {"step_fast", 32}
                        }}
                    }},
                    {"batchSize", {
                        {"type", "integer"},
                        {"title", "Batch Size"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 1},
                            {"step_fast", 4},
                            {"min", 1},
                            {"max", 16}
                        }}
                    }}
                }}
            };
        }

        int latentWidth = 512;
        int latentHeight = 512;
        int batchSize = 1;

        // Override the GetPropertyMap method
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["latentWidth"] = &latentWidth;
            properties["latentHeight"] = &latentHeight;
            properties["batchSize"] = &batchSize;
            return properties;
        }

        LatentComponent& operator=(const LatentComponent& other) {
            if (this != &other) { // Self-assignment check
                latentWidth = other.latentWidth;
                latentHeight = other.latentHeight;
                batchSize = other.batchSize;
            }
            return *this;
        }

        // Serialize the component to JSON
        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"latentWidth", latentWidth},
                {"latentHeight", latentHeight},
                {"batchSize", batchSize}
            };
            return j;
        }

        // Deserialize the component from JSON
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

            if (componentData.contains("latentWidth"))
                latentWidth = componentData["latentWidth"];
            if (componentData.contains("latentHeight"))
                latentHeight = componentData["latentHeight"];
            if (componentData.contains("batchSize"))
                batchSize = componentData["batchSize"];
        }
    };
}