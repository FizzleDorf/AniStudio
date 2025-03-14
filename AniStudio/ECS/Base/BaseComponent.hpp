// BaseComponent.hpp
#pragma once

#include "Types.hpp"
#include "nlohmann/json.hpp"

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