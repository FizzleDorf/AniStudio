#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <vector>

namespace ECS {

    struct HiresComponent : public BaseComponent {
        bool enabled = false;
        enum sd_hires_upscaler_t upscaler = SD_HIRES_UPSCALER_LATENT;
        std::string model_path;
        float scale = 2.0f;
        int target_width = 0;
        int target_height = 0;
        int steps = 10;
        float denoising_strength = 0.5f;
        int upscale_tile_size = 512;
        std::vector<float> custom_sigmas;

        HiresComponent() {
            compName = "Hires";
            compCategory = "Upscaling";

            std::vector<std::string> upscalerNames;
            for (int i = 0; i < SD_HIRES_UPSCALER_COUNT; ++i) {
                const char* name = sd_hires_upscaler_name(static_cast<enum sd_hires_upscaler_t>(i));
                if (name) upscalerNames.push_back(name);
            }

            schema = {
                {"title", "High Resolution Fix"},
                {"type", "object"},
                {"propertyOrder", {"enabled", "upscaler", "model_path", "scale", "target_width", "target_height", "steps", "denoising_strength", "upscale_tile_size", "custom_sigmas"}},
                {"properties", {
                    {"enabled", {
                        {"type", "boolean"},
                        {"title", "Enable HiRes Fix"},
                        {"default", false}
                    }},
                    {"upscaler", {
                        {"type", "integer"},
                        {"title", "Upscaler"},
                        {"ui:widget", "combo"},
                        {"enum", upscalerNames},
                        {"default", 0}
                    }},
                    {"model_path", {
                        {"type", "string"},
                        {"title", "Upscale Model Path"},
                        {"ui:widget", "file_selector"},
                        {"ui:options", {
                            {"mode", "file"},
                            {"filters", ".safetensors,.ckpt,.pt,.pth"},
                            {"filterName", "Upscale Models"},
                            {"buttonText", "Browse..."},
                            {"resetButtonText", "Clear"}
                        }}
                    }},
                    {"scale", {
                        {"type", "number"},
                        {"title", "Upscale Factor"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.1f}, {"min", 1.0f}, {"max", 8.0f}}}
                    }},
                    {"target_width", {
                        {"type", "integer"},
                        {"title", "Target Width"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 64}, {"max", 4096}}}
                    }},
                    {"target_height", {
                        {"type", "integer"},
                        {"title", "Target Height"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 64}, {"max", 4096}}}
                    }},
                    {"steps", {
                        {"type", "integer"},
                        {"title", "HiRes Steps"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 150}}}
                    }},
                    {"denoising_strength", {
                        {"type", "number"},
                        {"title", "Denoising Strength"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"upscale_tile_size", {
                        {"type", "integer"},
                        {"title", "Upscale Tile Size"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 64}, {"max", 1024}}}
                    }},
                    {"custom_sigmas", {
                        {"type", "string"},
                        {"title", "Custom Sigmas"},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"enabled", &enabled},
                {"upscaler", reinterpret_cast<int*>(&upscaler)},
                {"model_path", &model_path},
                {"scale", &scale},
                {"target_width", &target_width},
                {"target_height", &target_height},
                {"steps", &steps},
                {"denoising_strength", &denoising_strength},
                {"upscale_tile_size", &upscale_tile_size},
                {"custom_sigmas", &custom_sigmas}
            };
        }

        HiresComponent& operator=(const HiresComponent& other) {
            if (this != &other) {
                enabled = other.enabled;
                upscaler = other.upscaler;
                model_path = other.model_path;
                scale = other.scale;
                target_width = other.target_width;
                target_height = other.target_height;
                steps = other.steps;
                denoising_strength = other.denoising_strength;
                upscale_tile_size = other.upscale_tile_size;
                custom_sigmas = other.custom_sigmas;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"enabled", enabled},
                {"upscaler", static_cast<int>(upscaler)},
                {"model_path", model_path},
                {"scale", scale},
                {"target_width", target_width},
                {"target_height", target_height},
                {"steps", steps},
                {"denoising_strength", denoising_strength},
                {"upscale_tile_size", upscale_tile_size},
                {"custom_sigmas", custom_sigmas}
            }} };
        }

        void Deserialize(const nlohmann::json& j) override {
            BaseComponent::Deserialize(j);
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

            if (componentData.contains("enabled")) enabled = componentData["enabled"];
            if (componentData.contains("upscaler"))
                upscaler = static_cast<enum sd_hires_upscaler_t>(componentData["upscaler"].get<int>());
            if (componentData.contains("model_path")) model_path = componentData["model_path"].get<std::string>();
            if (componentData.contains("scale")) scale = componentData["scale"];
            if (componentData.contains("target_width")) target_width = componentData["target_width"];
            if (componentData.contains("target_height")) target_height = componentData["target_height"];
            if (componentData.contains("steps")) steps = componentData["steps"];
            if (componentData.contains("denoising_strength")) denoising_strength = componentData["denoising_strength"];
            if (componentData.contains("upscale_tile_size")) upscale_tile_size = componentData["upscale_tile_size"];
            if (componentData.contains("custom_sigmas") && componentData["custom_sigmas"].is_array())
                custom_sigmas = componentData["custom_sigmas"].get<std::vector<float>>();
        }
    };

} // namespace ECS