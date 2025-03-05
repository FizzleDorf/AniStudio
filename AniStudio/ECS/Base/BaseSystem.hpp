#pragma once
#include "Types.hpp"
#include <set>
#include <string>

namespace ECS {

// Forward declaration
class EntityManager;

class BaseSystem {
public:
    BaseSystem(EntityManager &entityMgr) : mgr(entityMgr), sysName("System_Component") {}

    virtual ~BaseSystem() = default;

    void RemoveEntity(const EntityID entity) { entities.erase(entity); }
    void AddEntity(const EntityID entity) { entities.insert(entity); }

    const EntitySignature GetSignature() const { return signature; }

    template <typename T>
    void AddComponentSignature() {
        signature.insert(CompType<T>());
    }

    virtual void Start() {}
    virtual void Update(const float deltaT) {}
    virtual void Destroy() {}

    std::string GetSystemName() { return sysName; }

protected:
    friend class EntityManager;
    EntitySignature signature;
    std::set<EntityID> entities;
    std::string sysName;
    EntityManager &mgr;
};

} // namespace ECS