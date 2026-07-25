#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <vector>

namespace ECS {

    struct SamplerComponent : public ECS::BaseComponent {
        SamplerComponent() {
            compName = "Sampler";
            compCategory = "Sampling";

            schema = {
                {"title", "Sampler Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "current_sample_method", "current_scheduler_method", "current_type_method",
                    "current_prediction_type",
                    "seed", "steps", "denoise", "n_threads", "free_params_immediately",
                    "offload_params_to_cpu", "keep_clip_on_cpu", "keep_control_net_on_cpu",
                    "extra_sample_args"
                }},
                {"properties", {
                    {"current_sample_method", {
                        {"type", "integer"},
                        {"title", "Sampler"},
                        {"description", "Sampling algorithm to use (Euler, DPM++, etc.)."},
                        {"ui:widget", "combo"},
                        {"items", sample_method_items},
                        {"itemCount", sample_method_item_count}
                    }},
                    {"current_scheduler_method", {
                        {"type", "integer"},
                        {"title", "Scheduler"},
                        {"description", "Noise schedule (Karras, Exponential, etc.)."},
                        {"ui:widget", "combo"},
                        {"items", scheduler_method_items},
                        {"itemCount", scheduler_method_item_count}
                    }},
                    {"current_type_method", {
                        {"type", "integer"},
                        {"title", "Quant Type"},
                        {"description", "Quantization type for model weights (FP16, Q4_K, etc.)."},
                        {"ui:widget", "combo"},
                        {"items", type_method_items},
                        {"itemCount", type_method_item_count}
                    }},
                    {"current_prediction_type", {
                        {"type", "integer"},
                        {"title", "Prediction Type"},
                        {"description", "Prediction type (EPS, V-Pred, Flow, etc.)."},
                        {"ui:widget", "combo"},
                        {"items", prediction_type_items},
                        {"itemCount", prediction_type_item_count}
                    }},
                    {"seed", {
                        {"type", "integer"},
                        {"title", "Seed"},
                        {"description", "Random seed for generation. -1 for random."},
                        {"ui:widget", "input_int"}
                    }},
                    {"steps", {
                        {"type", "integer"},
                        {"title", "Steps"},
                        {"description", "Number of sampling steps. Higher = better quality but slower."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 1}, {"max", 150}}}
                    }},
                    {"denoise", {
                        {"type", "number"},
                        {"title", "Denoise"},
                        {"description", "Denoising strength (0.0-1.0). Controls how much to change the image."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"n_threads", {
                        {"type", "integer"},
                        {"title", "# Threads"},
                        {"description", "Number of CPU threads to use for sampling."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 1}, {"max", 32}}}
                    }},
                    {"free_params_immediately", {
                        {"type", "boolean"},
                        {"title", "Free Params"},
                        {"description", "Free model parameters immediately after use to save memory."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"offload_params_to_cpu", {
                        {"type", "boolean"},
                        {"title", "Offload to CPU"},
                        {"description", "Offload model parameters to CPU when not in use to save VRAM."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"keep_clip_on_cpu", {
                        {"type", "boolean"},
                        {"title", "CLIP on CPU"},
                        {"description", "Keep CLIP model on CPU to save VRAM (slower)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"keep_control_net_on_cpu", {
                        {"type", "boolean"},
                        {"title", "ControlNet on CPU"},
                        {"description", "Keep ControlNet models on CPU to save VRAM (slower)."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"extra_sample_args", {
                        {"type", "string"},
                        {"title", "Extra Sample Args"},
                        {"description", "Additional arguments to pass to the sampler (advanced)."},
                        {"ui:widget", "text"}
                    }}
                }}
            };
        }

        int seed = -1;
        int steps = 20;
        float denoise = 1.0f;
        int n_threads = 4;
        bool free_params_immediately = true;
        bool offload_params_to_cpu = false;
        bool keep_clip_on_cpu = false;
        bool keep_control_net_on_cpu = false;

        sample_method_t current_sample_method = EULER_SAMPLE_METHOD;
        scheduler_t current_scheduler_method = DISCRETE_SCHEDULER;
        sd_type_t current_type_method = SD_TYPE_F16;
        prediction_t current_prediction_type = EPS_PRED;

        std::string extra_sample_args;

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"seed", &seed},
                {"steps", &steps},
                {"denoise", &denoise},
                {"n_threads", &n_threads},
                {"free_params_immediately", &free_params_immediately},
                {"offload_params_to_cpu", &offload_params_to_cpu},
                {"keep_clip_on_cpu", &keep_clip_on_cpu},
                {"keep_control_net_on_cpu", &keep_control_net_on_cpu},
                {"current_sample_method", reinterpret_cast<int*>(&current_sample_method)},
                {"current_scheduler_method", reinterpret_cast<int*>(&current_scheduler_method)},
                {"current_type_method", reinterpret_cast<int*>(&current_type_method)},
                {"current_prediction_type", reinterpret_cast<int*>(&current_prediction_type)},
                {"extra_sample_args", &extra_sample_args}
            };
        }

        SamplerComponent& operator=(const SamplerComponent& other) {
            if (this != &other) {
                seed = other.seed;
                steps = other.steps;
                denoise = other.denoise;
                n_threads = other.n_threads;
                free_params_immediately = other.free_params_immediately;
                offload_params_to_cpu = other.offload_params_to_cpu;
                keep_clip_on_cpu = other.keep_clip_on_cpu;
                keep_control_net_on_cpu = other.keep_control_net_on_cpu;
                current_sample_method = other.current_sample_method;
                current_scheduler_method = other.current_scheduler_method;
                current_type_method = other.current_type_method;
                current_prediction_type = other.current_prediction_type;
                extra_sample_args = other.extra_sample_args;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"seed", seed},
                {"steps", steps},
                {"denoise", denoise},
                {"n_threads", n_threads},
                {"free_params_immediately", free_params_immediately},
                {"offload_params_to_cpu", offload_params_to_cpu},
                {"keep_clip_on_cpu", keep_clip_on_cpu},
                {"keep_control_net_on_cpu", keep_control_net_on_cpu},
                {"current_sample_method", static_cast<int>(current_sample_method)},
                {"current_scheduler_method", static_cast<int>(current_scheduler_method)},
                {"current_type_method", static_cast<int>(current_type_method)},
                {"current_prediction_type", static_cast<int>(current_prediction_type)},
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

            if (componentData.contains("seed")) seed = componentData["seed"];
            if (componentData.contains("steps")) steps = componentData["steps"];
            if (componentData.contains("denoise")) denoise = componentData["denoise"];
            if (componentData.contains("n_threads")) n_threads = componentData["n_threads"];
            if (componentData.contains("free_params_immediately"))
                free_params_immediately = componentData["free_params_immediately"].get<bool>();
            if (componentData.contains("offload_params_to_cpu"))
                offload_params_to_cpu = componentData["offload_params_to_cpu"].get<bool>();
            if (componentData.contains("keep_clip_on_cpu"))
                keep_clip_on_cpu = componentData["keep_clip_on_cpu"].get<bool>();
            if (componentData.contains("keep_control_net_on_cpu"))
                keep_control_net_on_cpu = componentData["keep_control_net_on_cpu"].get<bool>();
            if (componentData.contains("current_sample_method"))
                current_sample_method = static_cast<sample_method_t>(componentData["current_sample_method"].get<int>());
            if (componentData.contains("current_scheduler_method"))
                current_scheduler_method = static_cast<scheduler_t>(componentData["current_scheduler_method"].get<int>());
            if (componentData.contains("current_type_method"))
                current_type_method = static_cast<sd_type_t>(componentData["current_type_method"].get<int>());
            if (componentData.contains("current_prediction_type"))
                current_prediction_type = static_cast<prediction_t>(componentData["current_prediction_type"].get<int>());
            if (componentData.contains("extra_sample_args"))
                extra_sample_args = componentData["extra_sample_args"].get<std::string>();
        }
    };

} // namespace ECS