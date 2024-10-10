#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct LoraComponent : public ECS::BaseComponent {
    std::string lora_reference;
    float strength; 
    float clipStrength;
    LoraComponent(const std::string &loraRef = "", float str = 1.0f) : lora_reference(loraRef), strength(str) {}
};
} 
