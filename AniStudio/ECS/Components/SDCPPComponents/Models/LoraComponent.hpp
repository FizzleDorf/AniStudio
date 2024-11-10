#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct LoraComponent : public ECS::BaseComponent {
    std::string loraName = "model.gguf";
    std::string loraPath = "";
    float loraStrength = 1.0f; 
    float loraClipStrength = 1.0f;

    LoraComponent &operator=(const LoraComponent &other) {
        if (this != &other) { // Self-assignment check
            loraName = other.loraName;
            loraPath = other.loraPath;
            loraStrength = other.loraStrength;
            loraStrength = other.loraStrength;
        }
        return *this;
    }
};
} 
