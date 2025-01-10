#pragma once
#include "ECS.h"
#include "ExampleComponent.hpp"

namespace ExamplePlugin {

class ExampleSystem : public ECS::BaseSystem {
public:
    ExampleSystem() {
        sysName = "ExampleSystem";
        AddComponentSignature<ExampleComponent>();
    }

    void Update(const float deltaT) override {
        for (const auto &entity : entities) {
            auto &counter = ECS::mgr.GetComponent<ExampleComponent>(entity);

            if (counter.autoIncrement) {
                counter.timeSinceLastUpdate += deltaT;

                // Check if it's time to update based on the rate
                if (counter.timeSinceLastUpdate >= (1.0f / counter.updateRate)) {
                    counter.count++;
                    counter.timeSinceLastUpdate = 0.0f;
                }
            }
        }
    }
};

} // namespace CounterPlugin