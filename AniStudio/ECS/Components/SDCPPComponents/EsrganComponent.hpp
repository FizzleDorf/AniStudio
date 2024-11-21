#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct EsrganComponent : public ECS::BaseComponent {
    std::string modelPath = "";
    std::string modelName = "model.gguf";
    float scale = 1.5;
    bool isEsrganLoaded = false;
};
}
