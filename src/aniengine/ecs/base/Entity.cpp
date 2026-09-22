#include "Entity.hpp"
#include "EntityManager.hpp"

#include <utility>

namespace ECS {

    template<typename T, typename... Args>
    void Entity::AddComponent(Args&&... args) {
        mgr->AddComponent<T>(ID, std::forward<Args>(args)...);
    }

    template<typename T>
    void Entity::AddComponent(T& component) {
        mgr->AddComponent<T>(ID, component);
    }

    template<typename T>
    T& Entity::GetComponent() {
        return mgr->GetComponent<T>(ID);
    }

    template<typename T>
    void Entity::RemoveComponent() {
        mgr->RemoveComponent<T>(ID);
    }

    template<typename T>
    bool Entity::HasComponent() {
        return mgr->HasComponent<T>(ID);
    }

    inline void Entity::Destroy() {
        mgr->DestroyEntity(ID);
    }
} // namespace ECS