#include "ExampleSystem.hpp"

namespace ECS {

ExampleSystem::ExampleSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) {
    sysName = "ExampleSystem";
    AddComponentSignature<ExampleComponent>();
}
ExampleSystem::~ExampleSystem() = default;

void ExampleSystem::Update(const float deltaT) {
    for (const auto &entity : entities) {
        auto &counter = mgr.GetComponent<ExampleComponent>(entity);
        if (counter.autoIncrement) {
            counter.timeSinceLastUpdate += deltaT;
            if (counter.timeSinceLastUpdate >= (1.0f / counter.updateRate)) {
                counter.count++;
                counter.timeSinceLastUpdate = 0.0f;
            }
        }
    }
}
} // namespace ECS