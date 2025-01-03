#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

// Base class for any loaded models
struct BaseModelComponent : public BaseComponent {
    BaseModelComponent() { compName = "BaseModelComponent"; }
    std::string modelPath = "";
    std::string modelName = "<none>";
    bool isModelLoaded = false;

    // Serialize to JSON
    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseComponent::Serialize();
        j["modelPath"] = modelPath;
        j["modelName"] = modelName;
        return j;
    }

    // Deserialize from JSON
    void Deserialize(const nlohmann::json &j) override {
        BaseComponent::Deserialize(j);
        if (j.contains("modelPath"))
            modelPath = j["modelPath"];
        if (j.contains("controlPath"))
            modelName = j["modelName"];
    }
};

// Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
struct ModelComponent : public BaseModelComponent {
    ModelComponent() { compName = "ModelComponent"; }
    ModelComponent &operator=(const ModelComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// Diffusion Model only (Flux)
struct DiffusionModelComponent : public BaseModelComponent {
    DiffusionModelComponent() { compName = "DiffusionModelComponent"; }
    DiffusionModelComponent &operator=(const DiffusionModelComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// Clip G Encoder
struct CLipGComponent : public BaseModelComponent {
    CLipGComponent() { compName = "CLipGComponent"; }
    CLipGComponent &operator=(const CLipGComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// Clip L Encoder
struct CLipLComponent : public BaseModelComponent {
    CLipLComponent() { compName = "CLipLComponent"; }
    CLipLComponent &operator=(const CLipLComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// T5 Encoder
struct T5XXLComponent : public BaseModelComponent {
    T5XXLComponent() { compName = "T5XXLComponent"; }
    T5XXLComponent &operator=(const T5XXLComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// Vae Loader (Overrides Model Component Vae)
struct VaeComponent : public BaseModelComponent {
    VaeComponent() { compName = "VaeComponent"; }
    bool isTiled = false;
    bool keep_vae_on_cpu = true;
    bool vae_decode_only = false;

    VaeComponent &operator=(const VaeComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
            isTiled = other.isTiled;
            keep_vae_on_cpu = other.keep_vae_on_cpu;
            vae_decode_only = other.vae_decode_only;
        }
        return *this;
    }
};

// Fast Vae Model loader
struct TaesdComponent : public BaseModelComponent {
    TaesdComponent() { compName = "TaesdComponent"; }
    TaesdComponent &operator=(const TaesdComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

// Low Rank Attention Model loader
struct LoraComponent : public ECS::BaseModelComponent {
    LoraComponent() { compName = "LoraComponent"; }
    float loraStrength = 1.0f;
    float loraClipStrength = 1.0f;

    LoraComponent &operator=(const LoraComponent &other) {
        if (this != &other) { // Self-assignment check
            modelName = other.modelName;
            modelPath = other.modelPath;
            isModelLoaded = other.isModelLoaded;
            loraStrength = other.loraStrength;
            loraStrength = other.loraStrength;
        }
        return *this;
    }
};

// Controlnet loader
struct ControlnetComponent : public ECS::BaseModelComponent {
    ControlnetComponent() { compName = "ControlnetComponent"; }
    float cnStrength = 1.0f;
    float applyStart = 0.0f;
    float applyEnd = 1.0f;

    // Operator = Overload for copying a component
    ControlnetComponent &operator=(const ControlnetComponent &other) {
        if (this != &other) {
            modelName = other.modelName;
            modelPath = other.modelPath;
            isModelLoaded = other.isModelLoaded;
            cnStrength = other.cnStrength;
            applyStart = other.applyStart;
            applyEnd = other.applyEnd;
        }
        return *this;
    }

    // Serialize to JSON
    nlohmann::json Serialize() const override {
        nlohmann::json j = BaseModelComponent::Serialize();
        j["cnStrength"] = cnStrength;
        j["applyStart"] = applyStart;
        j["applyEnd"] = applyEnd;
        return j;
    }

    // Deserialize from JSON
    void Deserialize(const nlohmann::json &j) override {
        BaseModelComponent::Deserialize(j);
        if (j.contains("cnStrength"))
            cnStrength = j["cnStrength"];
        if (j.contains("applyStart"))
            applyStart = j["applyStart"];
        if (j.contains("applyEnd"))
            applyEnd = j["applyEnd"];
    }
};

// Upscale models
struct EsrganComponent : public BaseModelComponent {
    EsrganComponent() { compName = "EsrganComponent"; }
    float scale = 1.5;

    EsrganComponent &operator=(const EsrganComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
            scale = other.scale;
        }
        return *this;
    }
};

struct EmbeddingComponent : public BaseModelComponent {
    EmbeddingComponent() { compName = "EmbeddingComponent"; }
    EmbeddingComponent &operator=(const EmbeddingComponent &other) {
        if (this != &other) {
            modelPath = other.modelPath;
            modelName = other.modelName;
            isModelLoaded = other.isModelLoaded;
        }
        return *this;
    }
};

} // namespace ECS
