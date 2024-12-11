#pragma once
#include "BaseComponent.hpp"
namespace ECS {
struct LatentTransformComponent : public ECS::BaseComponent {
    int newLatentWidth = 1024;
    int newLatentHeight = 1024;
    float latentWidthMultiplier = 1.5f;
    float latentHeightMultiplier = 1.5f;
};
} // namespace ECS