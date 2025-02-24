#pragma once
#include "BaseComponent.hpp"
#include "BaseSystem.hpp"
#include "CompList.hpp"
#include "Types.hpp"
#include "pch.h"

namespace ECS {

class EntityManager {
public:
    // Constructor: Initializes the EntityManager with a pool of available entity IDs.
    EntityManager() : entityCount(0) {
        // Preallocate entity IDs (0 to MAX_ENTITY_COUNT - 1) and add them to the availableEntities queue.
        for (EntityID entity = 0u; entity < MAX_ENTITY_COUNT; entity++) {
            availableEntities.push(entity);
        }
    }

    // Destructor: Clean up resources if needed.
    ~EntityManager() {}

    // Register a component type by name.
    // This maps the component's name to its ComponentTypeID and registers it in the ECS.
    template <typename T>
    void RegisterComponent(const std::string &name) {
        // Get the unique ComponentTypeID for the component type T.
        ComponentTypeID typeId = CompType<T>();

        // Map the ComponentTypeID to the component name.
        componentNames[typeId] = name;

        // Map the component name to the ComponentTypeID.
        nameToTypeId[name] = typeId;

        // Register the component type in the ECS.
        RegisterComponentType(
            typeId,
            [this](EntityID entity) { this->AddComponent<T>(entity); }, // Creator: Adds the component to an entity.
            [this](EntityID entity) -> BaseComponent * {
                if (this->HasComponent<T>(entity)) {
                    return &this->GetComponent<T>(entity); // Getter: Retrieves the component from an entity.
                }
                return nullptr;
            });

        // Add a CompList for the component type if it doesn't exist.
        if (componentsArrays.count(typeId) == 0) {
            componentsArrays[typeId] = std::move(std::make_shared<CompList<T>>());
        }
    }

    // Get the name of a component by its ComponentTypeID.
    std::string GetComponentName(ComponentTypeID typeId) const {
        auto it = componentNames.find(typeId);
        if (it != componentNames.end()) {
            return it->second;
        }
        return "Unknown";
    }

    // Get the ComponentTypeID by name.
    ComponentTypeID GetComponentTypeIdByName(const std::string &name) const {
        auto it = nameToTypeId.find(name);
        if (it != nameToTypeId.end()) {
            return it->second;
        }
        throw std::runtime_error("Component name not found");
    }

    // Retrieve a component by type (templated).
    template <typename T>
    T &GetComponent(EntityID entity) {
        ComponentTypeID typeId = CompType<T>();
        auto it = componentsArrays.find(typeId);
        if (it != componentsArrays.end()) {
            BaseComponent *component = it->second->Get(entity);
            if (component) {
                return *static_cast<T *>(component);
            }
        }
        throw std::runtime_error("Component not found for the given entity and type");
    }

    // Retrieve a component by name.
    BaseComponent &GetComponentByName(EntityID entity, const std::string &name) {
        ComponentTypeID typeId = GetComponentTypeIdByName(name);
        return GetComponentByTypeId(entity, typeId);
    }

    // Retrieve a component by ComponentTypeID.
    BaseComponent &GetComponentByTypeId(EntityID entity, ComponentTypeID typeId) {
        auto it = componentsArrays.find(typeId);
        if (it != componentsArrays.end()) {
            BaseComponent *component = it->second->Get(entity);
            if (component) {
                return *component;
            }
        }
        throw std::runtime_error("Component not found for the given entity and type ID");
    }

private:
    // Map ComponentTypeID to component name.
    std::unordered_map<ComponentTypeID, std::string> componentNames;

    // Map component name to ComponentTypeID.
    std::unordered_map<std::string, ComponentTypeID> nameToTypeId;

    // Queue of available entity IDs.
    std::queue<EntityID> availableEntities;

    // Map of entity IDs to their signatures.
    std::map<EntityID, std::shared_ptr<EntitySignature>> entitiesSignatures;

    // Map of system type IDs to registered systems.
    std::map<SystemTypeID, std::shared_ptr<BaseSystem>> registeredSystems;

    // Map of ComponentTypeIDs to their corresponding CompList.
    std::map<ComponentTypeID, std::shared_ptr<ICompList>> componentsArrays;

    // Map of ComponentTypeIDs to component creators.
    std::unordered_map<ComponentTypeID, ComponentCreator> componentCreators;

    // Map of ComponentTypeIDs to component getters.
    std::unordered_map<ComponentTypeID, ComponentGetter> componentGetters;

    // Current number of active entities.
    EntityID entityCount;

    // Register a component type with a creator and getter.
    void RegisterComponentType(ComponentTypeID typeId, ComponentCreator creator, ComponentGetter getter) {
        componentCreators[typeId] = creator;
        componentGetters[typeId] = getter;
    }

    // Get or create the signature for an entity.
    std::shared_ptr<EntitySignature> GetOrCreateEntitySignature(const EntityID entity) {
        if (entitiesSignatures.find(entity) == entitiesSignatures.end()) {
            entitiesSignatures[entity] = std::move(std::make_shared<EntitySignature>());
        }
        return entitiesSignatures.at(entity);
    }

    // Update the systems that an entity belongs to.
    void UpdateEntityTargetSystem(const EntityID entity) {
        auto signature = GetOrCreateEntitySignature(entity);
        for (auto &system : registeredSystems) {
            if (IsEntityInSystem(signature, system.second->signature)) {
                system.second->entities.insert(entity);
            } else {
                system.second->entities.erase(entity);
            }
        }
    }

    // Check if an entity matches a system's signature.
    bool IsEntityInSystem(const std::shared_ptr<EntitySignature> &entitySignature,
                          const EntitySignature &systemSignature) {
        for (const auto compType : systemSignature) {
            if (entitySignature->count(compType) == 0) {
                return false;
            }
        }
        return true;
    }
};

} // namespace ECS