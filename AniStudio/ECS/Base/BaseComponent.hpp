#pragma once

#include "Types.hpp"
#include "nlohmann/json.hpp"

namespace ECS {
struct BaseComponent {
    std::string compName = "Base_Component";
    std::string compCategory = "";

    // JSON UI definition - public so it can be modified by plugins
    nlohmann::json uiSchema = {};

    BaseComponent() : entityID() {}
    virtual ~BaseComponent() {}

    inline const EntityID GetID() const { return entityID; }

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
