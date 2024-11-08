#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct PromptComponent : public ECS::BaseComponent {
    std::string posPrompt = "";
    std::string negPrompt = "";
    char PosBuffer[9999] = "Positive";
    char NegBuffer[9999] = "Negative";

    PromptComponent &operator=(const PromptComponent &other) {
        if (this != &other) { // Self-assignment check
            posPrompt = other.posPrompt;
            negPrompt = other.negPrompt;
        }
        return *this;
    }

};
}
