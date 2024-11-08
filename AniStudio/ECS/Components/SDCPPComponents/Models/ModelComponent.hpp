#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {
struct ModelComponent : public ECS::BaseComponent {
    std::string modelPath = "";
    std::string modelName = "model.gguf";
    bool isCkptLoaded = false;

    ModelComponent &operator=(const ModelComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isCkptLoaded = other.isCkptLoaded;
        }
        return *this;
    }

    ModelComponent() = default;

};
} // namespace ECS
