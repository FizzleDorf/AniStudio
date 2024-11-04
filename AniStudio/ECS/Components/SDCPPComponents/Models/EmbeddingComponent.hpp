#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct EmbeddingComponent : public ECS::BaseComponent {
    std::string embedPath = "";
    std::string embedName = "model.gguf";
    bool isCkptLoaded = false;
};
} // namespace ECS