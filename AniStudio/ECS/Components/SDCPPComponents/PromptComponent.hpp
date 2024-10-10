#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct PromptComponent : public ECS::BaseComponent {
    std::string posPrompt;
    std::string negPrompt;

    PromptComponent(const std::string &newPosPrompt = "", const std::string &newNegPrompt = "")
        : posPrompt(newPosPrompt), negPrompt(newNegPrompt) {}
};
}
