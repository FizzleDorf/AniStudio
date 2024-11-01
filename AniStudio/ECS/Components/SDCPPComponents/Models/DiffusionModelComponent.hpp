#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct DiffusionModelComponent : public ECS::BaseComponent {
    std::string ckptPath = "";
    std::string ckptName = "model.gguf";
    bool isCkptLoaded = false;
};
}
