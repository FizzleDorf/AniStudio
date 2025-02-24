#pragma once

#include "EntityManager.hpp"

namespace ECS {

// Entity class represents an entity in the Entity-Component-System (ECS) architecture.
// An entity is essentially a unique identifier that can have components attached to it.
class Entity {

public:
    // Constructor: Initializes the Entity with a unique ID and a pointer to the EntityManager.
    // The EntityManager is responsible for managing the entity and its components.
    Entity(const EntityID id, EntityManager *manager) : ID(id), MGR(manager) {}

    // Destructor: Default destructor. No special cleanup is needed.
    ~Entity() = default;

    // GetID: Returns the unique ID of the entity.
    // This ID is used to identify the entity within the ECS.
    const EntityID GetID() const { return ID; }

    // AddComponent (variadic template): Adds a component of type T to the entity.
    // This version forwards arguments to the component's constructor.
    // Args: Variadic template arguments that are forwarded to the component's constructor.
    template <typename T, typename... Args>
    void AddComponent(Args &&...args) {
        MGR->AddComponent<T>(ID, std::forward<Args>(args)...);
    }

    // AddComponent (reference version): Adds a component of type T to the entity.
    // This version takes an existing component object and assigns it to the entity.
    // component: A reference to an existing component object.
    template <typename T>
    void AddComponent(T &component) {
        MGR->AddComponent<T>(ID, component);
    }

    // GetComponent: Retrieves a reference to the component of type T attached to the entity.
    // If the component does not exist, an exception is thrown.
    template <typename T>
    inline T &GetComponent() {
        return MGR->GetComponent<T>(ID);
    }

    // RemoveComponent: Removes the component of type T from the entity.
    // If the component does not exist, this method does nothing.
    template <typename T>
    inline void RemoveComponent() {
        return MGR->RemoveComponent<T>(ID);
    }

    // HasComponent: Checks if the entity has a component of type T.
    // Returns true if the component exists, false otherwise.
    template <typename T>
    inline bool HasComponent() {
        return MGR->HasComponent<T>(ID);
    }

    // Destroy: Destroys the entity and removes it from the EntityManager.
    // This also removes all components attached to the entity.
    void Destroy() { MGR->DestroyEntity(ID); }

private:
    // ID: The unique identifier for the entity.
    EntityID ID;

    // MGR: A pointer to the EntityManager that manages this entity.
    // The EntityManager is responsible for adding, removing, and querying components.
    EntityManager *MGR;
};
} // namespace ECS