#pragma once
#include "ECS.h"
#include "ExampleComponent.hpp"

namespace ECS {

class ANI_API ExampleSystem : public BaseSystem {
public:
    ExampleSystem(EntityManager &entityMgr);
    virtual ~ExampleSystem() override;
    void Update(const float deltaT) override;
};

} // namespace ECS