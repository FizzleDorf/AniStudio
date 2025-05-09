#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include <string>

namespace ECS {

    // Base class for any loaded models
    struct BaseModelComponent : public BaseComponent {
        BaseModelComponent() { compName = "BaseModelComponent"; }
        std::string modelPath = "";
        std::string modelName = "";
        bool isModelLoaded = false;

        nlohmann::json Serialize() const override {
            return { {compName, {{"modelName", modelName}}} };
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName"))
                modelName = componentData["modelName"];
        }
    };

    // Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
    struct ModelComponent : public BaseModelComponent {
        ModelComponent() { compName = "Model"; }
        ModelComponent& operator=(const ModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::checkpointDir + "\\" + modelName;
            }
        }
    };

    // Diffusion Model only (Flux)
    struct DiffusionModelComponent : public BaseModelComponent {
        DiffusionModelComponent() { compName = "DiffusionModel"; }
        DiffusionModelComponent& operator=(const DiffusionModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::unetDir + "\\" + modelName;
            }
        }
    };

    // Clip G Encoder
    struct ClipGComponent : public BaseModelComponent {
        ClipGComponent() { compName = "ClipG"; }
        ClipGComponent& operator=(const ClipGComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::encoderDir + "\\" + modelName;
            }
        }
    };

    // Clip L Encoder
    struct ClipLComponent : public BaseModelComponent {
        ClipLComponent() { compName = "ClipL"; }
        ClipLComponent& operator=(const ClipLComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::encoderDir + "\\" + modelName;
            }
        }
    };

    // T5 Encoder
    struct T5XXLComponent : public BaseModelComponent {
        T5XXLComponent() { compName = "T5XXL"; }
        T5XXLComponent& operator=(const T5XXLComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::encoderDir + "\\" + modelName;
            }
        }
    };

    // Vae Loader (Overrides Model Component Vae)
    struct VaeComponent : public BaseModelComponent {
        VaeComponent() { compName = "Vae"; }
        bool isTiled = false;
        bool keep_vae_on_cpu = false;
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
            return { {compName, {
                {"modelName", modelName},
                {"modelPath", modelPath},
                {"isTiled", isTiled},
                {"keep_vae_on_cpu", keep_vae_on_cpu},
                {"vae_decode_only", vae_decode_only}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName"))
                modelName = componentData["modelName"];
            if (componentData.contains("modelPath"))
                modelPath = componentData["modelPath"];
            if (componentData.contains("isTiled"))
                isTiled = componentData["isTiled"].get<bool>();
            if (componentData.contains("keep_vae_on_cpu"))
                keep_vae_on_cpu = componentData["keep_vae_on_cpu"].get<bool>();
            if (componentData.contains("vae_decode_only"))
                vae_decode_only = componentData["vae_decode_only"].get<bool>();

            if (!modelName.empty() && modelPath.empty())
                modelPath = Utils::FilePaths::vaeDir + "\\" + modelName;
        }
    };

    // Fast Vae Model loader
    struct TaesdComponent : public BaseModelComponent {
        TaesdComponent() { compName = "Taesd"; }
        TaesdComponent& operator=(const TaesdComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
        nlohmann::json Serialize() const override {
            return { {compName, {{"modelName", modelName}}} };
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::vaeDir + "\\" + modelName;
            }
        }
    };

    // Low Rank Attention Model loader
    struct LoraComponent : public ECS::BaseModelComponent {
        LoraComponent() { compName = "Lora"; }
        float loraStrength = 1.0f;
        float loraClipStrength = 1.0f;

        LoraComponent& operator=(const LoraComponent& other) {
            if (this != &other) { // Self-assignment check
                modelName = other.modelName;
                modelPath = other.modelPath;
                isModelLoaded = other.isModelLoaded;
                loraStrength = other.loraStrength;
                loraClipStrength = other.loraClipStrength;
            }
            return *this;
        }
        nlohmann::json Serialize() const override {
            return { {compName, {
                {"modelName", modelName},
                {"loraStrength", loraStrength},
                {"loraClipStrength", loraClipStrength}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName"))
                modelName = componentData["modelName"];
            if (componentData.contains("loraStrength"))
                loraStrength = componentData["loraStrength"];
            if (componentData.contains("loraClipStrength"))
                loraClipStrength = componentData["loraClipStrength"];
            if (!modelName.empty())
                modelPath = Utils::FilePaths::loraDir + "\\" + modelName;
        }
    };

    // Controlnet loader
    struct ControlnetComponent : public ECS::BaseModelComponent {
        ControlnetComponent() { compName = "Controlnet"; }
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
            return { {compName, {
                {"modelName", modelName},
                {"cnStrength", cnStrength},
                {"applyStart", applyStart},
                {"applyEnd", applyEnd}
            }} };
        }

        // Deserialize from JSON
        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName"))
                modelName = componentData["modelName"];
            if (componentData.contains("cnStrength"))
                cnStrength = componentData["cnStrength"];
            if (componentData.contains("applyStart"))
                applyStart = componentData["applyStart"];
            if (componentData.contains("applyEnd"))
                applyEnd = componentData["applyEnd"];
            if (!modelName.empty())
                modelPath = Utils::FilePaths::controlnetDir + "\\" + modelName;
        }
    };

    // Upscale models
    struct EsrganComponent : public BaseModelComponent {
        EsrganComponent() { compName = "Esrgan"; }
        uint32_t upscaleFactor = 2; // Default 2x upscaling
        bool preserveAspectRatio = true;

        EsrganComponent& operator=(const EsrganComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
                upscaleFactor = other.upscaleFactor;
                preserveAspectRatio = other.preserveAspectRatio;
            }
            return *this;
        }
        nlohmann::json Serialize() const override {
            return { {compName, {
                {"modelName", modelName},
                {"upscaleFactor", upscaleFactor},
                {"preserveAspectRatio",preserveAspectRatio}
            }} };
        }
        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName"))
                modelName = componentData["modelName"];
            if (componentData.contains("upscaleFactor"))
                upscaleFactor = componentData["upscaleFactor"];
            if (componentData.contains("preserveAspectRatio"))
                preserveAspectRatio = componentData["preserveAspectRatio"].get<bool>();
            if (!modelName.empty())
                modelPath = Utils::FilePaths::upscaleDir + "\\" + modelName;
        }
    };

    //Embedding
    struct EmbeddingComponent : public BaseModelComponent {
        EmbeddingComponent() { compName = "EmbeddingComponent"; }
        EmbeddingComponent& operator=(const EmbeddingComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::embedDir + "\\" + modelName;
            }
        }
    };

    // Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
    struct StackedIDComponent : public BaseModelComponent {
        StackedIDComponent() { compName = "Model"; }
        StackedIDComponent& operator=(const StackedIDComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }

        void Deserialize(const nlohmann::json& j) override {
            nlohmann::json componentData;

            if (j.contains(compName)) {
                componentData = j.at(compName);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == compName) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("modelName")) {
                modelName = componentData["modelName"];
                if (!modelName.empty())
                    modelPath = Utils::FilePaths::checkpointDir + "\\" + modelName;
            }
        }
    };

} // namespace ECS