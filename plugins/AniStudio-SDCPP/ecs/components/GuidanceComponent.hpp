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
        bool enable_slg = false;
        float slg_scale = 1.0f;
        float slg_layer_start = 0.0f;
        float slg_layer_end = 1.0f;
        std::string slg_layers = "";
        int scheduler = 0;
        int sample_method = 0;
        int sample_steps = 20;
        float eta = 0.0f;
        int shifted_timestep = -1;
        std::string custom_sigmas = "";
        float flow_shift = 0.0f;
        std::string extra_sample_args = "";

        GuidanceComponent() {
            compName = "Guidance";
            compCategory = "Sampling";

            schema = {
                {"title", "Guidance & Sampling Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "txt_cfg", "img_cfg", "distilled_guidance",
                    "enable_slg", "slg_scale", "slg_layer_start", "slg_layer_end", "slg_layers",
                    "scheduler", "sample_method", "sample_steps", "eta", "shifted_timestep",
                    "custom_sigmas", "flow_shift", "extra_sample_args"
                }},
                {"properties", {
                    {"txt_cfg", {
                        {"type", "number"},
                        {"title", "Text CFG"},
                        {"description", "Classifier-free guidance scale for text conditioning"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"img_cfg", {
                        {"type", "number"},
                        {"title", "Image CFG"},
                        {"description", "Classifier-free guidance scale for image conditioning"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"distilled_guidance", {
                        {"type", "number"},
                        {"title", "Distilled Guidance"},
                        {"description", "Guidance scale for distilled/consistency models"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 30.0f}}}
                    }},
                    {"enable_slg", {
                        {"type", "boolean"},
                        {"title", "Enable SLG"},
                        {"description", "Enable skip-layer guidance"},
                        {"default", false}
                    }},
                    {"slg_scale", {
                        {"type", "number"},
                        {"title", "SLG Scale"},
                        {"description", "Strength of skip-layer guidance"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 10.0f}}}
                    }},
                    {"slg_layer_start", {
                        {"type", "number"},
                        {"title", "SLG Layer Start"},
                        {"description", "First layer fraction for SLG (0.0-1.0)"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"slg_layer_end", {
                        {"type", "number"},
                        {"title", "SLG Layer End"},
                        {"description", "Last layer fraction for SLG (0.0-1.0)"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"slg_layers", {
                        {"type", "string"},
                        {"title", "SLG Layers"},
                        {"description", "Comma-separated list of specific layer indices to apply SLG"},
                        {"ui:widget", "textarea"}
                    }},
                    {"scheduler", {
                        {"type", "integer"},
                        {"title", "Scheduler"},
                        {"description", "Noise schedule for sampling"},
                        {"ui:widget", "combo"},
                        {"items", scheduler_method_items},
                        {"itemCount", scheduler_method_item_count},
                        {"default", 0}
                    }},
                    {"sample_method", {
                        {"type", "integer"},
                        {"title", "Sample Method"},
                        {"description", "Numerical integration method for diffusion sampling"},
                        {"ui:widget", "combo"},
                        {"items", sample_method_items},
                        {"itemCount", sample_method_item_count},
                        {"default", 0}
                    }},
                    {"sample_steps", {
                        {"type", "integer"},
                        {"title", "Sample Steps"},
                        {"description", "Number of sampling steps"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 1000}}}
                    }},
                    {"eta", {
                        {"type", "number"},
                        {"title", "Eta"},
                        {"description", "Stochasticity parameter (0 = deterministic, 1 = full random)"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"shifted_timestep", {
                        {"type", "integer"},
                        {"title", "Shifted Timestep"},
                        {"description", "Offset timestep value for diffusion process"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -1}, {"max", 1000}}}
                    }},
                    {"custom_sigmas", {
                        {"type", "string"},
                        {"title", "Custom Sigmas"},
                        {"description", "User-defined sigma schedule (comma-separated values)"},
                        {"ui:widget", "textarea"}
                    }},
                    {"flow_shift", {
                        {"type", "number"},
                        {"title", "Flow Shift"},
                        {"description", "Flow matching shift parameter"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 10.0f}}}
                    }},
                    {"extra_sample_args", {
                        {"type", "string"},
                        {"title", "Extra Sample Args"},
                        {"description", "Additional sampler arguments as key=value pairs"},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"txt_cfg", &txt_cfg},
                {"img_cfg", &img_cfg},
                {"distilled_guidance", &distilled_guidance},
                {"enable_slg", &enable_slg},
                {"slg_scale", &slg_scale},
                {"slg_layer_start", &slg_layer_start},
                {"slg_layer_end", &slg_layer_end},
                {"slg_layers", &slg_layers},
                {"scheduler", &scheduler},
                {"sample_method", &sample_method},
                {"sample_steps", &sample_steps},
                {"eta", &eta},
                {"shifted_timestep", &shifted_timestep},
                {"custom_sigmas", &custom_sigmas},
                {"flow_shift", &flow_shift},
                {"extra_sample_args", &extra_sample_args}
            };
        }

        GuidanceComponent& operator=(const GuidanceComponent& other) {
            if (this != &other) {
                txt_cfg = other.txt_cfg;
                img_cfg = other.img_cfg;
                distilled_guidance = other.distilled_guidance;
                enable_slg = other.enable_slg;
                slg_scale = other.slg_scale;
                slg_layer_start = other.slg_layer_start;
                slg_layer_end = other.slg_layer_end;
                slg_layers = other.slg_layers;
                scheduler = other.scheduler;
                sample_method = other.sample_method;
                sample_steps = other.sample_steps;
                eta = other.eta;
                shifted_timestep = other.shifted_timestep;
                custom_sigmas = other.custom_sigmas;
                flow_shift = other.flow_shift;
                extra_sample_args = other.extra_sample_args;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"txt_cfg", txt_cfg},
                {"img_cfg", img_cfg},
                {"distilled_guidance", distilled_guidance},
                {"enable_slg", enable_slg},
                {"slg_scale", slg_scale},
                {"slg_layer_start", slg_layer_start},
                {"slg_layer_end", slg_layer_end},
                {"slg_layers", slg_layers},
                {"scheduler", scheduler},
                {"sample_method", sample_method},
                {"sample_steps", sample_steps},
                {"eta", eta},
                {"shifted_timestep", shifted_timestep},
                {"custom_sigmas", custom_sigmas},
                {"flow_shift", flow_shift},
                {"extra_sample_args", extra_sample_args}
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

            if (componentData.contains("txt_cfg")) txt_cfg = componentData["txt_cfg"];
            if (componentData.contains("img_cfg")) img_cfg = componentData["img_cfg"];
            if (componentData.contains("distilled_guidance")) distilled_guidance = componentData["distilled_guidance"];
            if (componentData.contains("enable_slg")) enable_slg = componentData["enable_slg"];
            if (componentData.contains("slg_scale")) slg_scale = componentData["slg_scale"];
            if (componentData.contains("slg_layer_start")) slg_layer_start = componentData["slg_layer_start"];
            if (componentData.contains("slg_layer_end")) slg_layer_end = componentData["slg_layer_end"];
            if (componentData.contains("slg_layers")) slg_layers = componentData["slg_layers"];
            if (componentData.contains("scheduler")) scheduler = componentData["scheduler"];
            if (componentData.contains("sample_method")) sample_method = componentData["sample_method"];
            if (componentData.contains("sample_steps")) sample_steps = componentData["sample_steps"];
            if (componentData.contains("eta")) eta = componentData["eta"];
            if (componentData.contains("shifted_timestep")) shifted_timestep = componentData["shifted_timestep"];
            if (componentData.contains("custom_sigmas")) custom_sigmas = componentData["custom_sigmas"];
            if (componentData.contains("flow_shift")) flow_shift = componentData["flow_shift"];
            if (componentData.contains("extra_sample_args")) extra_sample_args = componentData["extra_sample_args"];
        }
    };

} // namespace ECS