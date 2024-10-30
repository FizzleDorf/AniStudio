#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct PromptComponent : public ECS::BaseComponent {
    std::string posPrompt = "Positive";
    std::string negPrompt = "Negative";
    char PosBuffer[9999] = "";
    char NegBuffer[9999] = "";
};
}
