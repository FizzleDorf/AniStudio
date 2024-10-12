#pragma once
#include "BaseComponent.hpp"
namespace ECS {
struct LatentUpscaleComponent : public ECS::BaseComponent {
    int newLatentWidth = 0;
    int newLatentHeight = 0;
    float latentWidthMultiplier = 1.0f;
    float latentHeightMultiplier = 1.0f;
};
} // namespace ECS