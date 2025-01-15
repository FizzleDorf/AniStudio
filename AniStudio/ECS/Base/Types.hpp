#pragma once
#include "DLLDefines.hpp"
#include <set>

namespace ECS {
class BaseSystem;
struct BaseComponent;

// Constants
const size_t MAX_ENTITY_COUNT = 5000;
const size_t MAX_COMPONENT_COUNT = 32;

// Custom Types
using EntityID = size_t;
using SystemTypeID = size_t;
using ComponentTypeID = size_t;
using EntitySignature = std::set<ComponentTypeID>;

// Non-template functions
static ComponentTypeID GetRuntimeComponentTypeID() {
    static ComponentTypeID typeID = 0u;
    return typeID++;
}

static SystemTypeID GetRuntimeSystemTypeID() {
    static SystemTypeID typeID = 0u;
    return typeID++;
}

// Template functions
template <typename T>
inline static const ComponentTypeID CompType() noexcept {
    static_assert((std::is_base_of<BaseComponent, T>::value && !std::is_same<BaseComponent, T>::value),
                  "INVALID COMPONENT TYPE");
    static const ComponentTypeID typeID = GetRuntimeComponentTypeID();
    return typeID;
}

template <typename T>
inline static const SystemTypeID SystemType() noexcept {
    static_assert((std::is_base_of<BaseSystem, T>::value && !std::is_same<BaseSystem, T>::value),
                  "INVALID COMPONENT TYPE");
    static const SystemTypeID typeID = GetRuntimeSystemTypeID();
    return typeID;
}
} // namespace ECS