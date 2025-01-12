#pragma once
#include "ECS.h"
#include "components.h"

namespace ECS {
class MeshSystem : public BaseSystem {
public:
    MeshSystem(EntityManager &entityMgr) : BaseSystem(entityMgr) {
        sysName = "MeshSystem";
        AddComponentSignature<MeshComponent>();
    }

    void Start() override {}
};
} // namespace ECS