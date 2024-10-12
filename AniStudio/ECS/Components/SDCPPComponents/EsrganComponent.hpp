#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"

namespace ECS {
struct EsrganComponent : public ECS::BaseComponent {
    std::string modelPath = "path/to/model";
    std::string modelName = "model.gguf";
    bool isEsrganLoaded = false;
};
}
