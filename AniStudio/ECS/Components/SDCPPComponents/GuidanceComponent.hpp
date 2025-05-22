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
#include <string>

namespace ECS {

    struct GuidanceComponent : public ECS::BaseComponent {
        GuidanceComponent() {
            compName = "Guidance";

            // Define the component schema
            schema = {
                {"title", "Guidance Settings"},
                {"type", "object"},
                {"propertyOrder", {"guidance", "eta"}},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 64.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"guidance", {
                        {"type", "number"},
                        {"title", "Guidance Scale"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.5f},
                            {"format", "%.2f"}
                        }}
                    }},
                    {"eta", {
                        {"type", "number"},
                        {"title", "ETA"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.05f},
                            {"step_fast", 0.1f},
                            {"format", "%.2f"}
                        }}
                    }}
                }}
            };
        }

        float guidance = 2.0f;
        float eta = 0.0f;

        // Override the GetPropertyMap method
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["guidance"] = &guidance;
            properties["eta"] = &eta;
            return properties;
        }

        GuidanceComponent& operator=(const GuidanceComponent& other) {
            if (this != &other) {
                guidance = other.guidance;
                eta = other.eta;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"guidance", guidance},
                      {"eta", eta}
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

            if (componentData.contains("guidance"))
                guidance = componentData["guidance"];
            if (componentData.contains("eta"))
                eta = componentData["eta"];
        }
    };

} // namespace ECS