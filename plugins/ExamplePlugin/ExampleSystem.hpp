#pragma once
#include "ECS/Base/BaseSystem.hpp"
#include "ECS/Base/EntityManager.hpp"
#include "ExampleComponent.hpp"

using namespace ECS;

namespace ExamplePlugin {

class ExampleSystem : public ECS::BaseSystem {
public:
    ExampleSystem() {
        sysName = "ExampleSystem";
        AddComponentSignature<ExampleComponent>();
    }
    
    void Update(const float deltaT) override {
        for (const auto& entity : entities) {
            auto& comp = ECS::mgr.GetComponent<ExampleComponent>(entity);
            comp.value = std::fmod(comp.value + deltaT, 1.0f);
        }
    }
};

} // namespace ExamplePlugin