#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

    struct CheckpointComponent : public BaseModelComponent {
        CheckpointComponent() {
            compName = "Checkpoint";
            schema = {
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
        }

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
        DiffusionModelComponent() {
            compName = "DiffusionModel";
            schema = {
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
        }

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
        HighNoiseDiffusionModelComponent() {
            compName = "HighNoiseDiffusionModel";
            schema = {
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
        UncondDiffusionModelComponent() {
            compName = "UncondDiffusionModel";
            schema = {
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
        MotionModuleComponent() {
            compName = "MotionModule";
            schema = {
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
        }

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