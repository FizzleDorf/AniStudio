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
};

struct DiffusionModelComponent : public ECS::BaseComponent {
    std::string ckptPath = "";
    std::string ckptName = "model.gguf";
    bool isCkptLoaded = false;

    DiffusionModelComponent &operator=(const DiffusionModelComponent &other) {
        if (this != &other) {
            ckptPath = other.ckptPath;
            ckptName = other.ckptName;
            isCkptLoaded = other.isCkptLoaded;
        }
        return *this;
    }
};
} // namespace ECS
