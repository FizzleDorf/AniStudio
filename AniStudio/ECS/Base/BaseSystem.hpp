#pragma once
#include "Types.hpp"
#include <set>
#include <string>

namespace ECS {

// Forward declaration of EntityManager to avoid circular dependencies
class EntityManager;

// BaseSystem class serves as the foundation for all systems in the Entity-Component-System (ECS) architecture.
class BaseSystem {
public:
    // Constructor initializes the system with a reference to the EntityManager and sets a default system name.
    BaseSystem(EntityManager &entityMgr) : mgr(entityMgr), sysName("System_Component") {}

    // Virtual destructor to allow for proper cleanup in derived classes.
    virtual ~BaseSystem() = default;

    // Removes an entity from the system's set of entities.
    void RemoveEntity(const EntityID entity) { entities.erase(entity); }

    // Adds an entity to the system's set of entities.
    void AddEntity(const EntityID entity) { entities.insert(entity); }

    // Returns the signature of the system, which defines the components required by the system.
    const EntitySignature GetSignature() const { return signature; }

    // Template method to add a component type to the system's signature.
    template <typename T>
    void AddComponentSignature() {
        signature.insert(CompType<T>());
    }

    // Virtual methods that can be overridden by derived systems.
    // Start is called when the system is first initialized.
    virtual void Start() {}

    // Update is called every frame, with deltaT representing the time since the last frame.
    virtual void Update(const float deltaT) {}

    // Render is called every frame, typically used for drawing or rendering operations.
    virtual void Render() {}

    // Destroy is called when the system is being destroyed or removed.
    virtual void Destroy() {}

    // Returns the name of the system.
    std::string GetSystemName() { return sysName; }

protected:
    // Friend class declaration to allow EntityManager to access protected members.
    friend class EntityManager;

    // The signature of the system, which defines the set of components that an entity must have to be processed by this
    // system.
    EntitySignature signature;

    // Set of entities that are currently being managed by this system.
    std::set<EntityID> entities;

    // Name of the system, which can be used for debugging or identification purposes.
    std::string sysName;

    // Reference to the EntityManager that this system is associated with.
    EntityManager &mgr;
};

} // namespace ECS