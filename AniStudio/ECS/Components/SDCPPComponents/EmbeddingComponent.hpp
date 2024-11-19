#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct EmbeddingComponent : public ECS::BaseComponent {
    std::string embedPath = "";
    std::string embedName = "model.gguf";
    bool isCkptLoaded = false;

    EmbeddingComponent &operator=(const EmbeddingComponent &other) {
        if (this != &other) {
            embedPath = other.embedPath;
            embedName = other.embedName;
            isCkptLoaded = other.isCkptLoaded;
        }
        return *this;
    }
};
} // namespace ECS