#pragma once

#include "BaseModelComponent.hpp"
#include <string>

namespace ECS {
struct ModelComponent : public ECS::BaseModelComponent {

    ModelComponent &operator=(const ModelComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

struct DiffusionModelComponent : public ECS::BaseModelComponent {

    DiffusionModelComponent &operator=(const DiffusionModelComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};
} // namespace ECS
