#pragma once
#include <set>

namespace ECS {

// Forward declarations for BaseSystem and BaseComponent to avoid circular dependencies.
class BaseSystem;
struct BaseComponent;

// Constants defining the maximum number of entities and components that can be managed.
const size_t MAX_ENTITY_COUNT = 5000;  // Maximum number of entities in the ECS.
const size_t MAX_COMPONENT_COUNT = 32; // Maximum number of component types in the ECS.

// Custom type definitions for ECS.
using EntityID = size_t;                           // Unique identifier for entities.
using SystemTypeID = size_t;                       // Unique identifier for system types.
using ComponentTypeID = size_t;                    // Unique identifier for component types.
using EntitySignature = std::set<ComponentTypeID>; // A set of component type IDs representing an entity's composition.

// Non-template function to generate a unique runtime component type ID.
// This ID is incremented each time the function is called.
static ComponentTypeID GetRuntimeComponentTypeID() {
    static ComponentTypeID typeID = 0u; // Static variable to ensure uniqueness across calls.
    return typeID++;
}

// Non-template function to generate a unique runtime system type ID.
// This ID is incremented each time the function is called.
static SystemTypeID GetRuntimeSystemTypeID() {
    static SystemTypeID typeID = 0u; // Static variable to ensure uniqueness across calls.
    return typeID++;
}

// Template function to get the unique component type ID for a given component type.
// This ensures that each component type has a unique ID at compile time.
template <typename T>
inline static const ComponentTypeID CompType() noexcept {
    // Static assertion to ensure that T is derived from BaseComponent and is not BaseComponent itself.
    static_assert((std::is_base_of<BaseComponent, T>::value && !std::is_same<BaseComponent, T>::value),
                  "INVALID COMPONENT TYPE");
    static const ComponentTypeID typeID = GetRuntimeComponentTypeID(); // Generate a unique ID for the component type.
    return typeID;
}

// Template function to get the unique system type ID for a given system type.
// This ensures that each system type has a unique ID at compile time.
template <typename T>
inline static const SystemTypeID SystemType() noexcept {
    // Static assertion to ensure that T is derived from BaseSystem and is not BaseSystem itself.
    static_assert((std::is_base_of<BaseSystem, T>::value && !std::is_same<BaseSystem, T>::value),
                  "INVALID SYSTEM TYPE");
    static const SystemTypeID typeID = GetRuntimeSystemTypeID(); // Generate a unique ID for the system type.
    return typeID;
}

} // namespace ECS