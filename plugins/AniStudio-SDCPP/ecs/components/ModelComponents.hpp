#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

    struct CheckpointComponent : public BaseModelComponent {
        CheckpointComponent() = default;

        const char* GetCompName() const override { return "Checkpoint"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Checkpoint Model"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "Checkpoint"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "Checkpoint Models"},
                            {"dialogDefaultPath", "checkpoint"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for checkpoint model files (.safetensors, .ckpt, .pt, .gguf)"}
                        }}
                    }}
                }}
            };
            return j;
        }

        CheckpointComponent(const CheckpointComponent& other) : BaseModelComponent(other) {}

        CheckpointComponent& operator=(const CheckpointComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct DiffusionModelComponent : public BaseModelComponent {
        DiffusionModelComponent() = default;

        const char* GetCompName() const override { return "DiffusionModel"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "UNet/Diffusion Model"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "UNet"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "UNet Models"},
                            {"dialogDefaultPath", "diffusion_model"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for UNet/Diffusion model files for FLUX or transformer models"}
                        }}
                    }}
                }}
            };
            return j;
        }

        DiffusionModelComponent(const DiffusionModelComponent& other) : BaseModelComponent(other) {}

        DiffusionModelComponent& operator=(const DiffusionModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct HighNoiseDiffusionModelComponent : public BaseModelComponent {
        HighNoiseDiffusionModelComponent() = default;

        const char* GetCompName() const override { return "HighNoiseDiffusionModel"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "High Noise UNet/Diffusion Model"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "High Noise UNet"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "High Noise UNet Models"},
                            {"dialogDefaultPath", "high_noise_diffusion_model"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for high noise UNet/Diffusion model files (for video generation)"}
                        }}
                    }}
                }}
            };
            return j;
        }

        HighNoiseDiffusionModelComponent(const HighNoiseDiffusionModelComponent& other)
            : BaseModelComponent(other) {
        }

        HighNoiseDiffusionModelComponent& operator=(const HighNoiseDiffusionModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct UncondDiffusionModelComponent : public BaseModelComponent {
        UncondDiffusionModelComponent() = default;

        const char* GetCompName() const override { return "UncondDiffusionModel"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Unconditional Diffusion Model"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "Uncond UNet"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "UNet Models"},
                            {"dialogDefaultPath", "uncond_diffusion_model"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for unconditional diffusion model files"}
                        }}
                    }}
                }}
            };
            return j;
        }

        UncondDiffusionModelComponent(const UncondDiffusionModelComponent& other)
            : BaseModelComponent(other) {
        }

        UncondDiffusionModelComponent& operator=(const UncondDiffusionModelComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct MotionModuleComponent : public BaseModelComponent {
        MotionModuleComponent() = default;

        const char* GetCompName() const override { return "MotionModule"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Motion Module"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "Motion Module"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "Motion Module Models"},
                            {"dialogDefaultPath", "motion_module"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for motion module files (for video generation)"}
                        }}
                    }}
                }}
            };
            return j;
        }

        MotionModuleComponent(const MotionModuleComponent& other) : BaseModelComponent(other) {}

        MotionModuleComponent& operator=(const MotionModuleComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

}