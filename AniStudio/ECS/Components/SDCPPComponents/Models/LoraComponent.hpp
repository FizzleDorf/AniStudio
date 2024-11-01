#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct LoraComponent : public ECS::BaseComponent {
    std::string loraPath = "";
    float loraStrength = 1.0f; 
    float loraClipStrength = 1.0f;
};
} 
