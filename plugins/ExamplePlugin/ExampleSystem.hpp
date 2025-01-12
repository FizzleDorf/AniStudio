#pragma once
#include "ECS.h"
#include "ExampleComponent.hpp"

namespace ECS {

class ExampleSystem : public ECS::BaseSystem {
public:
    ExampleSystem(ECS::EntityManager &entityMgr) : BaseSystem(entityMgr) {
        sysName = "ExampleSystem";
        AddComponentSignature<ExampleComponent>();
    }

    void Update(const float deltaT) override {
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
};

} // namespace ECS