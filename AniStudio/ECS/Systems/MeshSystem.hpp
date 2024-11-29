#include "BaseSystem.hpp"
#include "components.h"

namespace ECS {
class MeshSystem : public BaseSystem {
public:
    MeshSystem() { AddComponentSignature<MeshComponent>(); }

    void Start() override{};
    void Update() override{};
};
} // namespace ECS