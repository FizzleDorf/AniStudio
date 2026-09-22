#pragma once

#include "Types.hpp"

namespace ECS {

    class EntityManager;

    class Entity {
    public:
        Entity(const EntityID id, EntityManager* manager) : ID(id), mgr(manager) {}
        ~Entity() = default;

        const EntityID GetID() const { return ID; }

        template<typename T, typename... Args>
        void AddComponent(Args&&... args);

        template<typename T>
        void AddComponent(T& component);  

        template<typename T>
        T& GetComponent();                

        template<typename T>
        void RemoveComponent();           

        template<typename T>
        bool HasComponent();              

        void Destroy();                   

    private:
        EntityID ID;
        EntityManager* mgr;
    };

} // namespace ECS