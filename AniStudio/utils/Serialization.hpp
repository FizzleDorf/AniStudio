#pragma once

#include "ECS.h"
#include "pch.h"

namespace ECS {

// Serialize a single component using the virtual method in BaseComponent
template <typename T>
nlohmann::json SerializeComponent(const EntityID entity) {
    assert(mgr.HasComponent<T>(entity) && "Entity does not have the specified component!");

    // Get the component of type T
    const T &component = mgr.GetComponent<T>(entity);

    // Call the component's specific Serialize method (which is virtual in the BaseComponent)
    return component.Serialize();
}

// Serialize all components of an entity into a JSON object
nlohmann::json SerializeEntityComponents(const EntityID entity) {
    nlohmann::json entityJson;

    // Loop through all component types of the entity
    const std::vector<ComponentTypeID> componentTypes = mgr.GetEntityComponents(entity);
    for (const ComponentTypeID &compType : componentTypes) {
        // Use a generic method for each component type based on the actual component stored
        mgr.ForEachComponent([&entityJson, &entity, &compType](auto &component) {
            using T = std::decay_t<decltype(component)>;

            // Check if the component type matches the current component type
            if (compType == CompType<T>()) {
                entityJson[typeid(T).name()] = SerializeComponent<T>(entity);
            }
        });
    }

    return entityJson;
}

// Deserialize a single component into an entity using the virtual method in BaseComponent
template <typename T>
void DeserializeComponent(EntityID entity, const nlohmann::json &componentJson) {
    T component;
    component.Deserialize(componentJson);

    // Add the component back to the entity
    mgr.AddComponent<T>(entity, std::move(component));
}

// Deserialize all components for an entity from a JSON object
void DeserializeEntityComponents(EntityID entity, const nlohmann::json &entityJson) {
    // Loop through the JSON object and deserialize components based on their type
    for (auto &[componentName, componentJsonData] : entityJson.items()) {
        // TODO: Deserialize for SDCPP API
    }
}

} // namespace ECS
