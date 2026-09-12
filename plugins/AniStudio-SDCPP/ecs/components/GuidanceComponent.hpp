#pragma once

#include "BaseComponent.hpp"
#include "DiffusionOptions.hpp"
#include "stable-diffusion.h"
#include <string>
#include <vector>

namespace ECS {

    struct GuidanceComponent : public ECS::BaseComponent {
        float txt_cfg = 7.0f;
        float img_cfg = 1.5f;
        float distilled_guidance = 3.5f;
        float eta = 0.0f;
        int shifted_timestep = -1;
        float flow_shift = 0.0f;

        GuidanceComponent() = default;

        const char* GetCompName() const override { return "Guidance"; }
        const char* GetCompCategory() const override { return "Sampling"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Guidance & Sampling Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "txt_cfg", "img_cfg", "distilled_guidance",
                    "eta", "shifted_timestep", "flow_shift"
                }},
                {"properties", {
                    {"txt_cfg", {
                        {"type", "number"},
                        {"title", "Text CFG"},
                        {"description", "Classifier-Free Guidance scale for text prompt. Higher = more adherence."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"img_cfg", {
                        {"type", "number"},
                        {"title", "Image CFG"},
                        {"description", "Guidance scale for image conditioning (img2img)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"distilled_guidance", {
                        {"type", "number"},
                        {"title", "Distilled Guidance"},
                        {"description", "Guidance scale for distilled models (e.g., LCM)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"eta", {
                        {"type", "number"},
                        {"title", "Eta"},
                        {"description", "Noise multiplier for DDIM sampler (0.0 = deterministic)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"shifted_timestep", {
                        {"type", "integer"},
                        {"title", "Shifted Timestep"},
                        {"description", "Shift timesteps for sampling (advanced)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -1}, {"max", 1000}}}
                    }},
                    {"flow_shift", {
                        {"type", "number"},
                        {"title", "Flow Shift"},
                        {"description", "Shift factor for flow-matching models."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 10.0f}}}
                    }}
                }}
            };
            return j;
        }

        GuidanceComponent(const GuidanceComponent& other) : BaseComponent(other) {
            txt_cfg = other.txt_cfg;
            img_cfg = other.img_cfg;
            distilled_guidance = other.distilled_guidance;
            eta = other.eta;
            shifted_timestep = other.shifted_timestep;
            flow_shift = other.flow_shift;
        }

        GuidanceComponent& operator=(const GuidanceComponent& other) {
            if (this != &other) {
                txt_cfg = other.txt_cfg;
                img_cfg = other.img_cfg;
                distilled_guidance = other.distilled_guidance;
                eta = other.eta;
                shifted_timestep = other.shifted_timestep;
                flow_shift = other.flow_shift;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"txt_cfg", &txt_cfg},
                {"img_cfg", &img_cfg},
                {"distilled_guidance", &distilled_guidance},
                {"eta", &eta},
                {"shifted_timestep", &shifted_timestep},
                {"flow_shift", &flow_shift}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"txt_cfg", txt_cfg},
                {"img_cfg", img_cfg},
                {"distilled_guidance", distilled_guidance},
                {"eta", eta},
                {"shifted_timestep", shifted_timestep},
                {"flow_shift", flow_shift}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == key) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("txt_cfg")) txt_cfg = componentData["txt_cfg"];
            if (componentData.contains("img_cfg")) img_cfg = componentData["img_cfg"];
            if (componentData.contains("distilled_guidance")) distilled_guidance = componentData["distilled_guidance"];
            if (componentData.contains("eta")) eta = componentData["eta"];
            if (componentData.contains("shifted_timestep")) shifted_timestep = componentData["shifted_timestep"];
            if (componentData.contains("flow_shift")) flow_shift = componentData["flow_shift"];
        }
    };

    struct SLGComponent : public ECS::BaseComponent {
        bool enable_slg = false;
        float slg_scale = 1.0f;
        float slg_layer_start = 0.0f;
        float slg_layer_end = 1.0f;
        std::string slg_layers = "";

        SLGComponent() = default;

        const char* GetCompName() const override { return "SLG"; }
        const char* GetCompCategory() const override { return "Sampling"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Skip Layer Guidance (SLG)"},
                {"type", "object"},
                {"propertyOrder", {
                    "enable_slg", "slg_scale", "slg_layer_start", "slg_layer_end", "slg_layers"
                }},
                {"properties", {
                    {"enable_slg", {
                        {"type", "boolean"},
                        {"title", "Enable SLG"},
                        {"description", "Enable Skip Layer Guidance for improved quality."},
                        {"default", false}
                    }},
                    {"slg_scale", {
                        {"type", "number"},
                        {"title", "SLG Scale"},
                        {"description", "Strength of Skip Layer Guidance."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 10.0f}}}
                    }},
                    {"slg_layer_start", {
                        {"type", "number"},
                        {"title", "SLG Layer Start"},
                        {"description", "Layer index to start applying SLG (0.0-1.0)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"slg_layer_end", {
                        {"type", "number"},
                        {"title", "SLG Layer End"},
                        {"description", "Layer index to stop applying SLG (0.0-1.0)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"slg_layers", {
                        {"type", "string"},
                        {"title", "SLG Layers"},
                        {"description", "Comma-separated list of layer indices for SLG."},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
            return j;
        }

        SLGComponent(const SLGComponent& other) : BaseComponent(other) {
            enable_slg = other.enable_slg;
            slg_scale = other.slg_scale;
            slg_layer_start = other.slg_layer_start;
            slg_layer_end = other.slg_layer_end;
            slg_layers = other.slg_layers;
        }

        SLGComponent& operator=(const SLGComponent& other) {
            if (this != &other) {
                enable_slg = other.enable_slg;
                slg_scale = other.slg_scale;
                slg_layer_start = other.slg_layer_start;
                slg_layer_end = other.slg_layer_end;
                slg_layers = other.slg_layers;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"enable_slg", &enable_slg},
                {"slg_scale", &slg_scale},
                {"slg_layer_start", &slg_layer_start},
                {"slg_layer_end", &slg_layer_end},
                {"slg_layers", &slg_layers}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"enable_slg", enable_slg},
                {"slg_scale", slg_scale},
                {"slg_layer_start", slg_layer_start},
                {"slg_layer_end", slg_layer_end},
                {"slg_layers", slg_layers}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == key) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("enable_slg")) enable_slg = componentData["enable_slg"];
            if (componentData.contains("slg_scale")) slg_scale = componentData["slg_scale"];
            if (componentData.contains("slg_layer_start")) slg_layer_start = componentData["slg_layer_start"];
            if (componentData.contains("slg_layer_end")) slg_layer_end = componentData["slg_layer_end"];
            if (componentData.contains("slg_layers")) slg_layers = componentData["slg_layers"];
        }
    };

    struct CustomSigmasComponent : public ECS::BaseComponent {
        std::vector<float> custom_sigmas;

        CustomSigmasComponent() = default;

        const char* GetCompName() const override { return "CustomSigmas"; }
        const char* GetCompCategory() const override { return "Sampling"; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Custom Sigmas"},
                {"type", "object"},
                {"propertyOrder", {"custom_sigmas"}},
                {"properties", {
                    {"custom_sigmas", {
                        {"type", "array"},
                        {"title", "Custom Sigmas"},
                        {"description", "Custom sigma schedule (advanced)."},
                        {"items", {"type", "number"}},
                        {"ui:widget", "array_input"}
                    }}
                }}
            };
            return j;
        }

        CustomSigmasComponent(const CustomSigmasComponent& other)
            : BaseComponent(other)
            , custom_sigmas(other.custom_sigmas) {
        }

        CustomSigmasComponent& operator=(const CustomSigmasComponent& other) {
            if (this != &other) {
                custom_sigmas = other.custom_sigmas;
            }
            return *this;
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"custom_sigmas", &custom_sigmas}
            };
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"custom_sigmas", custom_sigmas}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key)) {
                componentData = j.at(key);
            }
            else {
                for (auto it = j.begin(); it != j.end(); ++it) {
                    if (it.key() == key) {
                        componentData = it.value();
                        break;
                    }
                }
                if (componentData.empty()) {
                    componentData = j;
                }
            }

            if (componentData.contains("custom_sigmas") && componentData["custom_sigmas"].is_array())
                custom_sigmas = componentData["custom_sigmas"].get<std::vector<float>>();
        }
    };

} // namespace ECS