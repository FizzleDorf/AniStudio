#pragma once

#include "BaseComponent.hpp"
#include "filepaths.hpp"
#include <string>

namespace ECS {

    // Base class for any loaded models
    struct BaseModelComponent : public BaseComponent {
        BaseModelComponent() { compName = "BaseModelComponent"; }
        std::string modelPath = "";
        std::string modelName = "<none>";
        bool isModelLoaded = false;

        nlohmann::json Serialize() const override { return { {compName, {{"modelName", modelName}}} }; }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
            }
        }
    };

    // Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
    struct ModelComponent : public BaseModelComponent {
        ModelComponent() { compName = "Model"; } // Changed from "ModelComponent" to "Model"
        ModelComponent& operator=(const ModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.checkpointDir + "\\" + modelName;
            }
        }
    };

    // Diffusion Model only (Flux)
    struct DiffusionModelComponent : public BaseModelComponent {
        DiffusionModelComponent() { compName = "DiffusionModel"; } // Changed from "DiffusionModelComponent" to "DiffusionModel"
        DiffusionModelComponent& operator=(const DiffusionModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.unetDir + "\\" + modelName;
            }
        }
    };

    // Clip G Encoder
    struct CLipGComponent : public BaseModelComponent {
        CLipGComponent() { compName = "CLipG"; } // Changed from "CLipGComponent" to "CLipG"
        CLipGComponent& operator=(const CLipGComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.encoderDir + "\\" + modelName;
            }
        }
    };

    // Clip L Encoder
    struct CLipLComponent : public BaseModelComponent {
        CLipLComponent() { compName = "CLipL"; } // Changed from "CLipLComponent" to "CLipL"
        CLipLComponent& operator=(const CLipLComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.encoderDir + "\\" + modelName;
            }
        }
    };

    // T5 Encoder
    struct T5XXLComponent : public BaseModelComponent {
        T5XXLComponent() { compName = "T5XXL"; } // Changed from "T5XXLComponent" to "T5XXL"
        T5XXLComponent& operator=(const T5XXLComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.encoderDir + "\\" + modelName;
            }
        }
    };

    // Vae Loader (Overrides Model Component Vae)
    struct VaeComponent : public BaseModelComponent {
        VaeComponent() { compName = "Vae"; } // Changed from "VaeComponent" to "Vae"
        bool isTiled = false;
        bool keep_vae_on_cpu = true;
        bool vae_decode_only = false;

        VaeComponent& operator=(const VaeComponent& other) {
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

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"modelName", modelName},
                      {"modelPath", modelPath},
                      {"isTiled", isTiled},
                      {"keep_vae_on_cpu", keep_vae_on_cpu},
                      {"vae_decode_only", vae_decode_only}}} };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                if (obj.contains("modelPath"))
                    modelPath = obj["modelPath"];
                if (obj.contains("isTiled"))
                    isTiled = obj["isTiled"].get<bool>();
                if (obj.contains("keep_vae_on_cpu"))
                    keep_vae_on_cpu = obj["keep_vae_on_cpu"].get<bool>();
                if (obj.contains("vae_decode_only"))
                    vae_decode_only = obj["vae_decode_only"].get<bool>();
            }
        }
    };

    // Fast Vae Model loader
    struct TaesdComponent : public BaseModelComponent {
        TaesdComponent() { compName = "Taesd"; } // Changed from "TaesdComponent" to "Taesd"
        TaesdComponent& operator=(const TaesdComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        nlohmann::json Serialize() const override { return { {compName, {{"modelName", modelName}}} }; }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.vaeDir + "\\" + modelName;
            }
        }
    };

    // Low Rank Attention Model loader
    struct LoraComponent : public ECS::BaseModelComponent {
        LoraComponent() { compName = "Lora"; } // Changed from "LoraComponent" to "Lora"
        float loraStrength = 1.0f;
        float loraClipStrength = 1.0f;

        LoraComponent& operator=(const LoraComponent& other) {
            if (this != &other) { // Self-assignment check
                modelName = other.modelName;
                modelPath = other.modelPath;
                isModelLoaded = other.isModelLoaded;
                loraStrength = other.loraStrength;
                loraClipStrength = other.loraClipStrength; // Fixed duplicate assignment
            }
            return *this;
        }
        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"modelName", modelName}, {"loraStrength", loraStrength}, {"loraClipStrength", loraClipStrength}}} };
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                if (obj.contains("loraStrength"))
                    loraStrength = obj["loraStrength"];
                if (obj.contains("loraClipStrength"))
                    loraClipStrength = obj["loraClipStrength"];
                modelPath = filePaths.loraDir + "\\" + modelName;
            }
        }
    };

    // Controlnet loader
    struct ControlnetComponent : public ECS::BaseModelComponent {
        ControlnetComponent() { compName = "Controlnet"; } // Changed from "ControlnetComponent" to "Controlnet"
        float cnStrength = 1.0f;
        float applyStart = 0.0f;
        float applyEnd = 1.0f;

        // Operator = Overload for copying a component
        ControlnetComponent& operator=(const ControlnetComponent& other) {
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

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"modelName", modelName},
                      {"cnStrength", cnStrength},
                      {"applyStart", applyStart},
                      {"applyEnd", applyEnd}}} };
        }

        // Deserialize from JSON
        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                if (obj.contains("cnStrength"))
                    cnStrength = obj["cnStrength"];
                if (obj.contains("applyStart"))
                    applyStart = obj["applyStart"];
                if (obj.contains("applyEnd"))
                    applyEnd = obj["applyEnd"];
                modelPath = filePaths.controlnetDir + "\\" + modelName;
            }
        }
    };

    // Upscale models
    struct EsrganComponent : public BaseModelComponent {
        EsrganComponent() { compName = "Esrgan"; } // Changed from "EsrganComponent" to "Esrgan"
        float scale = 1.5;

        EsrganComponent& operator=(const EsrganComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
                scale = other.scale;
            }
            return *this;
        }
        nlohmann::json Serialize() const override { return { {compName, {{"modelName", modelName}, {"scale", scale}}} }; }
        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                if (obj.contains("scale"))
                    scale = obj["scale"];
                modelPath = filePaths.upscaleDir + "\\" + modelName;
            }
        }
    };

    struct EmbeddingComponent : public BaseModelComponent {
        EmbeddingComponent() { compName = "EmbeddingComponent"; } // Note: This matches the JSON format already
        EmbeddingComponent& operator=(const EmbeddingComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains(compName)) {
                const auto& obj = j.at(compName);
                if (obj.contains("modelName"))
                    modelName = obj["modelName"];
                modelPath = filePaths.embedDir + "\\" + modelName;
            }
        }
    };

} // namespace ECS