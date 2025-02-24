#pragma once

#include "ComponentMetadata.hpp"
#include "Types.hpp"
#include "nlohmann/json.hpp"

namespace ECS {

// BaseComponent is the base struct for all components in the Entity-Component-System (ECS) architecture.
// It inherits from IComponentType, which likely provides a common interface for all components.
struct BaseComponent : public IComponentType {
    // Default name for the component. Derived components can override this.
    std::string compName = "Base_Component";

    // Virtual destructor to allow for proper cleanup in derived components.
    virtual ~BaseComponent() = default;

    // Returns the EntityID associated with this component.
    inline const EntityID GetID() const { return entityID; }

    // Provides metadata about the component, such as its name and category.
    // Derived components can override this to provide specific metadata.
    virtual ComponentMetadata GetMetadata() const override {
        return ComponentMetadata{.name = compName, .category = "Base"};
    }

    // Renders the component's properties in an editor or debug UI.
    // This is a default implementation that can be overridden by derived components.
    virtual void RenderProperties(EntityManager &mgr, EntityID entity) override {
        ImGui::Text("Component: %s", compName.c_str());
    }

    // Executes logic associated with the component.
    // This is a default implementation that can be overridden by derived components.
    virtual void Execute(EntityManager &mgr, EntityID entity) override {}

    // Serializes the component's data into a JSON object.
    // Derived components can override this to include additional data.
    virtual nlohmann::json Serialize() const {
        nlohmann::json j;
        j["compName"] = compName;
        return j;
    }

    // Deserializes the component's data from a JSON object.
    // Derived components can override this to handle additional data.
    virtual void Deserialize(const nlohmann::json &j) {
        if (j.contains("compName"))
            compName = j["compName"];
    }

private:
    // Friend declaration to allow EntityManager to access the private entityID.
    friend class EntityManager;

    // The EntityID associated with this component.
    // This is set by the EntityManager when the component is attached to an entity.
    EntityID entityID;
};

} // namespace ECS