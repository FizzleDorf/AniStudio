#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct ModelComponent : public ECS::BaseComponent {
    std::string modelPath = "";
    std::string modelName = "model.gguf";
    bool isCkptLoaded = false;
};
} // namespace ECS
