#pragma once

#include "Types.hpp"
#include "nlohmann/json.hpp"

namespace ECS {
struct BaseComponent {
    std::string compName = "Base_Component";
    BaseComponent() : entityID() {}
    virtual ~BaseComponent() {}

    inline const EntityID GetID() const { return entityID; }

    // Serialize to JSON
    virtual nlohmann::json Serialize() const {
        nlohmann::json j;
        j["compName"] = compName;
        // j["entityID"] = entityID;
        return j;
    }

    // Deserialize from JSON
    virtual void Deserialize(const nlohmann::json &j) {
        if (j.contains("compName"))
            compName = j["compName"];
        // if (j.contains("entityID"))
        //     entityID = j["entityID"];
    }

private:
    friend class EntityManager; // Friend class
    EntityID entityID;
};
} // namespace ECS
