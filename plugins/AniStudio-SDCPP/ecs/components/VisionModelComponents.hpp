#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

    struct ClipVisionComponent : public BaseModelComponent {
        ClipVisionComponent() = default;

        const char* GetCompName() const override { return "ClipVision"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "CLIP Vision Encoder"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "CLIP Vision"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt"},
                            {"filterName", "CLIP Vision Models"},
                            {"dialogDefaultPath", "Encoder"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for CLIP Vision encoder files (for img2img, etc.)"}
                        }}
                    }}
                }}
            };
            return j;
        }

        ClipVisionComponent(const ClipVisionComponent& other) : BaseModelComponent(other) {}

        ClipVisionComponent& operator=(const ClipVisionComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

    struct LlmVisionComponent : public BaseModelComponent {
        LlmVisionComponent() = default;

        const char* GetCompName() const override { return "LlmVision"; }
        const char* GetCompCategory() const override { return "Model"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "LLM Vision Encoder"},
                {"type", "object"},
                {"propertyOrder", {"modelPath"}},
                {"properties", {
                    {"modelPath", {
                        {"type", "string"},
                        {"title", "LLM Vision"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.gguf"},
                            {"filterName", "LLM Vision Models"},
                            {"dialogDefaultPath", "Encoder"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"},
                            {"browseTooltip", "Browse for LLM vision encoder files"}
                        }}
                    }}
                }}
            };
            return j;
        }

        LlmVisionComponent(const LlmVisionComponent& other) : BaseModelComponent(other) {}

        LlmVisionComponent& operator=(const LlmVisionComponent& other) {
            if (this != &other) {
                modelPath = other.modelPath;
                modelName = other.modelName;
                isModelLoaded = other.isModelLoaded;
            }
            return *this;
        }
    };

}