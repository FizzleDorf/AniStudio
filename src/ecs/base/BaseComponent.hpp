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

#include "Types.hpp"
#include "nlohmann/json.hpp"
#include <unordered_map>
#include <variant>
#include <string>

struct ImVec2;
struct ImVec4;

namespace UISchema {
    // Forward declaration of PropertyVariant
    using PropertyVariant = std::variant<
        bool*,
        int*,
        float*,
        double*,
        std::string*,
        ImVec2*,
        ImVec4*,
        std::vector<std::string>*
    >;
}

namespace ECS {
    struct BaseComponent {
        std::string compName = "Base_Component";
        std::string compCategory = "";

        // Single schema definition for node, inputs, outputs, and UI
        nlohmann::json schema = {};

        BaseComponent() : entityID() {}
        virtual ~BaseComponent() {}

        inline const EntityID GetID() const { return entityID; }

        // Get node schema part
        nlohmann::json getNodeSchema() const {
            nlohmann::json nodeSchema = {
                {"inputs", nlohmann::json::array()},
                {"outputs", nlohmann::json::array()}
            };

            if (schema.contains("title"))
                nodeSchema["title"] = schema["title"];

            if (schema.contains("inputs"))
                nodeSchema["inputs"] = schema["inputs"];

            if (schema.contains("outputs"))
                nodeSchema["outputs"] = schema["outputs"];

            return nodeSchema;
        }

        // Get UI schema part
        nlohmann::json getUISchema() const {
            nlohmann::json uiSchema = {
                {"type", "object"},
                {"properties", {}}
            };

            if (schema.contains("properties"))
                uiSchema["properties"] = schema["properties"];

            return uiSchema;
        }

        // Get property map for UI rendering - to be overridden by derived components
        virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() {
            return {}; // Empty map by default
        }

        // Serialize to JSON
        virtual nlohmann::json Serialize() const {
            nlohmann::json j;
            j["compName"] = compName;
            return j;
        }

        // Deserialize from JSON
        virtual void Deserialize(const nlohmann::json& j) {
            if (j.contains("compName"))
                compName = j["compName"];
        }

    private:
        friend class EntityManager; // Friend class
        EntityID entityID;
    };
} // namespace ECS