#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <vector>

namespace ECS {

    struct SamplerComponent : public ECS::BaseComponent {
        int seed = -1;
        int steps = 20;
        float eta = 0.0f;
        float denoise = 1.0f;
        int shifted_timestep = 0;
        std::vector<float> custom_sigmas;
        std::string extra_sample_args;

        enum sample_method_t sample_method = EULER_SAMPLE_METHOD;
        enum scheduler_t scheduler = DISCRETE_SCHEDULER;
        enum prediction_t prediction = EPS_PRED;

        SamplerComponent() {
            compName = "Sampler";
            compCategory = "Sampling";

            schema = {
                {"title", "Sampler Settings"},
                {"type", "object"},
                {"propertyOrder", {"sample_method", "scheduler", "prediction", "seed", "steps", "eta", "denoise", "shifted_timestep", "custom_sigmas", "extra_sample_args"}},
                {"properties", {
                    {"sample_method", {
                        {"type", "integer"},
                        {"title", "Sampler"},
                        {"description", "Sampling algorithm."},
                        {"ui:widget", "combo"},
                        {"items", sample_method_items},
                        {"itemCount", sample_method_item_count},
                        {"default", 0}
                    }},
                    {"scheduler", {
                        {"type", "integer"},
                        {"title", "Scheduler"},
                        {"description", "Noise schedule to use."},
                        {"ui:widget", "combo"},
                        {"items", scheduler_method_items},
                        {"itemCount", scheduler_method_item_count},
                        {"default", 0}
                    }},
                    {"prediction", {
                        {"type", "integer"},
                        {"title", "Prediction Type"},
                        {"description", "Prediction target type."},
                        {"ui:widget", "combo"},
                        {"items", prediction_type_items},
                        {"itemCount", prediction_type_item_count},
                        {"default", 0}
                    }},
                    {"seed", {
                        {"type", "integer"},
                        {"title", "Seed"},
                        {"description", "Random seed for generation (-1 = random)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", -1}, {"max", 2147483647}}}
                    }},
                    {"steps", {
                        {"type", "integer"},
                        {"title", "Steps"},
                        {"description", "Number of sampling steps."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 1}, {"max", 150}}}
                    }},
                    {"eta", {
                        {"type", "number"},
                        {"title", "ETA"},
                        {"description", "Eta parameter for stochastic samplers (0.0 = deterministic)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.05f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"denoise", {
                        {"type", "number"},
                        {"title", "Denoise"},
                        {"description", "Denoising strength for img2img (0.0?1.0)."},
                        {"ui:widget", "input_float"},
                        {"ui:options", {{"step", 0.01f}, {"min", 0.0f}, {"max", 1.0f}}}
                    }},
                    {"shifted_timestep", {
                        {"type", "integer"},
                        {"title", "Shifted Timestep"},
                        {"description", "Shift timestep value (0 = default)."},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"min", 0}, {"max", 1000}}}
                    }},
                    {"custom_sigmas", {
                        {"type", "string"},
                        {"title", "Custom Sigmas"},
                        {"description", "Comma?separated list of custom sigma values."},
                        {"ui:widget", "textarea"}
                    }},
                    {"extra_sample_args", {
                        {"type", "string"},
                        {"title", "Extra Sample Args"},
                        {"description", "Additional arguments passed to the sampler."},
                        {"ui:widget", "textarea"}
                    }}
                }}
            };
        }

        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            return {
                {"seed", &seed},
                {"steps", &steps},
                {"eta", &eta},
                {"denoise", &denoise},
                {"shifted_timestep", &shifted_timestep},
                {"custom_sigmas", &custom_sigmas},
                {"extra_sample_args", &extra_sample_args},
                {"sample_method", reinterpret_cast<int*>(&sample_method)},
                {"scheduler", reinterpret_cast<int*>(&scheduler)},
                {"prediction", reinterpret_cast<int*>(&prediction)}
            };
        }

        SamplerComponent& operator=(const SamplerComponent& other) {
            if (this != &other) {
                seed = other.seed;
                steps = other.steps;
                eta = other.eta;
                denoise = other.denoise;
                shifted_timestep = other.shifted_timestep;
                custom_sigmas = other.custom_sigmas;
                extra_sample_args = other.extra_sample_args;
                sample_method = other.sample_method;
                scheduler = other.scheduler;
                prediction = other.prediction;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName, {
                {"seed", seed},
                {"steps", steps},
                {"eta", eta},
                {"denoise", denoise},
                {"shifted_timestep", shifted_timestep},
                {"custom_sigmas", custom_sigmas},
                {"extra_sample_args", extra_sample_args},
                {"sample_method", static_cast<int>(sample_method)},
                {"scheduler", static_cast<int>(scheduler)},
                {"prediction", static_cast<int>(prediction)}
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
            if (componentData.contains("eta")) eta = componentData["eta"];
            if (componentData.contains("denoise")) denoise = componentData["denoise"];
            if (componentData.contains("shifted_timestep")) shifted_timestep = componentData["shifted_timestep"];
            if (componentData.contains("custom_sigmas") && componentData["custom_sigmas"].is_array())
                custom_sigmas = componentData["custom_sigmas"].get<std::vector<float>>();
            if (componentData.contains("extra_sample_args")) extra_sample_args = componentData["extra_sample_args"].get<std::string>();
            if (componentData.contains("sample_method"))
                sample_method = static_cast<enum sample_method_t>(componentData["sample_method"].get<int>());
            if (componentData.contains("scheduler"))
                scheduler = static_cast<enum scheduler_t>(componentData["scheduler"].get<int>());
            if (componentData.contains("prediction"))
                prediction = static_cast<enum prediction_t>(componentData["prediction"].get<int>());
        }
    };

} // namespace ECS