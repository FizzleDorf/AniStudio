/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once
#include "BaseComponent.hpp"
namespace ECS {
    struct LayerSkipComponent : public ECS::BaseComponent {
        LayerSkipComponent() {
            compName = "LayerSkip";

            // Define the component schema
            schema = {
                {"title", "Layer Skip Settings"},
                {"type", "object"},
                {"propertyOrder", {"slg_scale", "skip_layer_start", "skip_layer_end"}},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 64.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"slg_scale", {
                        {"type", "number"},
                        {"title", "SLG Scale"},
                        {"ui:widget", "drag_float"},
                        {"ui:options", {
                            {"min", 0.0f},
                            {"max", 1.0f},
                            {"speed", 0.01f},
                            {"format", "%.2f"}
                        }}
                    }},
                    {"skip_layer_start", {
                        {"type", "number"},
                        {"title", "Start"},
                        {"ui:widget", "drag_float"},
                        {"ui:options", {
                            {"min", 0.0f},
                            {"max", 1.0f},
                            {"speed", 0.01f},
                            {"format", "%.2f"}
                        }}
                    }},
                    {"skip_layer_end", {
                        {"type", "number"},
                        {"title", "End"},
                        {"ui:widget", "drag_float"},
                        {"ui:options", {
                            {"min", 0.0f},
                            {"max", 1.0f},
                            {"speed", 0.01f},
                            {"format", "%.2f"}
                        }}
                    }}
                }}
            };
        }

        int* skip_layers = 0;
        size_t skip_layers_count = 0;
        float slg_scale = 0.0f;
        float skip_layer_start = 0.0f;
        float skip_layer_end = 1.0f;

        // Override the GetPropertyMap method
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["slg_scale"] = &slg_scale;
            properties["skip_layer_start"] = &skip_layer_start;
            properties["skip_layer_end"] = &skip_layer_end;
            // Note: skip_layers pointer array is not exposed in the UI due to complexity
            return properties;
        }

        LayerSkipComponent& operator=(const LayerSkipComponent& other) {
            if (this != &other) { // Self-assignment check
                skip_layers = other.skip_layers;
                skip_layers_count = other.skip_layers_count;
                slg_scale = other.slg_scale;
                skip_layer_start = other.skip_layer_start;
                skip_layer_end = other.skip_layer_end;
            }
            return *this;
        }

        // Serialize the component to JSON
        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j[compName] = {
                {"slg_scale", slg_scale},
                {"skip_layer_start", skip_layer_start},
                {"skip_layer_end", skip_layer_end}
                // Note: skip_layers array not serialized due to complexity
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

            if (componentData.contains("slg_scale"))
                slg_scale = componentData["slg_scale"];
            if (componentData.contains("skip_layer_start"))
                skip_layer_start = componentData["skip_layer_start"];
            if (componentData.contains("skip_layer_end"))
                skip_layer_end = componentData["skip_layer_end"];
            // Note: skip_layers array not deserialized due to complexity
        }
    };
} // namespace ECS