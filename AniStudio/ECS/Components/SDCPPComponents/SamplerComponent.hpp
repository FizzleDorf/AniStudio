#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "Constants.hpp"
#include <string>

namespace ECS {

    struct SamplerComponent : public ECS::BaseComponent {
        SamplerComponent() {
            compName = "Sampler";

            // Define the component schema
            schema = {
                {"title", "Sampler Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "current_sample_method", "current_scheduler_method", "seed",
                    "cfg", "steps", "denoise", "n_threads", "free_params_immediately"
                }},
                {"ui:table", {
                    {"columns", 2},
                    {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
                    {"columnSetup", {
                        {"Param", ImGuiTableColumnFlags_WidthFixed, 64.0f},
                        {"Value", ImGuiTableColumnFlags_WidthStretch}
                    }}
                }},
                {"properties", {
                    {"current_sample_method", {
                        {"type", "integer"},
                        {"title", "Sampler"},
                        {"ui:widget", "combo"},
                        {"items", sample_method_items},
                        {"itemCount", sample_method_item_count}
                    }},
                    {"current_scheduler_method", {
                        {"type", "integer"},
                        {"title", "Scheduler"},
                        {"ui:widget", "combo"},
                        {"items", scheduler_method_items},
                        {"itemCount", scheduler_method_item_count}
                    }},
                    {"current_type_method", {
                        {"type", "integer"},
                        {"title", "Quant Type"},
                        {"ui:widget", "combo"},
                        {"items", type_method_items},
                        {"itemCount", type_method_item_count}
                    }},
                    {"current_rng_type", {
                        {"type", "integer"},
                        {"title", "RNG Type"},
                        {"ui:widget", "combo"},
                        {"items", type_rng_items},
                        {"itemCount", type_rng_item_count}
                    }},
                    {"seed", {
                        {"type", "integer"},
                        {"title", "Seed"},
                        {"ui:widget", "input_int"}
                    }},
                    {"cfg", {
                        {"type", "number"},
                        {"title", "CFG"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.5f},
                            {"step_fast", 1.0f},
                            {"format", "%.2f"}
                        }}
                    }},
                    {"steps", {
                        {"type", "integer"},
                        {"title", "Steps"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 1},
                            {"step_fast", 5},
                            {"min", 1},
                            {"max", 150}
                        }}
                    }},
                    {"denoise", {
                        {"type", "number"},
                        {"title", "Denoise"},
                        {"ui:widget", "input_float"},
                        {"ui:options", {
                            {"step", 0.01f},
                            {"step_fast", 0.1f},
                            {"format", "%.2f"},
                            {"min", 0.0f},
                            {"max", 1.0f}
                        }}
                    }},
                    {"n_threads", {
                        {"type", "integer"},
                        {"title", "# Threads"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {
                            {"step", 1},
                            {"step_fast", 4},
                            {"min", 1},
                            {"max", 32}
                        }}
                    }},
                    {"free_params_immediately", {
                        {"type", "boolean"},
                        {"title", "Free Params"},
                        {"ui:widget", "checkbox"}
                    }}
                }}
            };
        }

        int steps = 20;
        float denoise = 1.0;
        float cfg = 7.0;
        int seed = -1;
        int n_threads = 4;
        bool free_params_immediately = true;

        sample_method_t current_sample_method = sample_method_t::EULER;
        schedule_t current_scheduler_method = schedule_t::DEFAULT;
        sd_type_t current_type_method = sd_type_t::SD_TYPE_F16;
        rng_type_t current_rng_type = rng_type_t::STD_DEFAULT_RNG;

        // Override the GetPropertyMap method
        std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
            std::unordered_map<std::string, UISchema::PropertyVariant> properties;
            properties["steps"] = &steps;
            properties["denoise"] = &denoise;
            properties["cfg"] = &cfg;
            properties["seed"] = &seed;
            properties["n_threads"] = &n_threads;
            properties["free_params_immediately"] = &free_params_immediately;

            // Need to use reinterpret_cast for the enum types
            properties["current_sample_method"] = reinterpret_cast<int*>(&current_sample_method);
            properties["current_scheduler_method"] = reinterpret_cast<int*>(&current_scheduler_method);
            properties["current_type_method"] = reinterpret_cast<int*>(&current_type_method);
            properties["current_rng_type"] = reinterpret_cast<int*>(&current_rng_type);

            return properties;
        }

        SamplerComponent& operator=(const SamplerComponent& other) {
            if (this != &other) {
                steps = other.steps;
                denoise = other.denoise;
                cfg = other.cfg;
                seed = other.seed;
                n_threads = other.n_threads;
                free_params_immediately = other.free_params_immediately;
                current_sample_method = other.current_sample_method;
                current_scheduler_method = other.current_scheduler_method;
                current_type_method = other.current_type_method;
                current_rng_type = other.current_rng_type;
            }
            return *this;
        }

        nlohmann::json Serialize() const override {
            return { {compName,
                     {{"seed", seed},
                      {"steps", steps},
                      {"cfg", cfg},
                      {"denoise", denoise},
                      {"n_threads", n_threads},
                      {"free_params_immediately", free_params_immediately},
                      {"current_sample_method", static_cast<int>(current_sample_method)},
                      {"current_scheduler_method", static_cast<int>(current_scheduler_method)},
                      {"current_type_method", static_cast<int>(current_type_method)},
                      {"current_rng_type", static_cast<int>(current_rng_type)}}} };
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

            if (componentData.contains("seed"))
                seed = componentData["seed"];
            if (componentData.contains("steps"))
                steps = componentData["steps"];
            if (componentData.contains("cfg"))
                cfg = componentData["cfg"];
            if (componentData.contains("denoise"))
                denoise = componentData["denoise"];
            if (componentData.contains("n_threads"))
                n_threads = componentData["n_threads"];
            if (componentData.contains("free_params_immediately"))
                free_params_immediately = componentData["free_params_immediately"].get<bool>();
            if (componentData.contains("current_sample_method"))
                current_sample_method = static_cast<sample_method_t>(componentData["current_sample_method"]);
            if (componentData.contains("current_scheduler_method"))
                current_scheduler_method = static_cast<schedule_t>(componentData["current_scheduler_method"]);
            if (componentData.contains("current_type_method"))
                current_type_method = static_cast<sd_type_t>(componentData["current_type_method"]);
            if (componentData.contains("current_rng_type"))
                current_rng_type = static_cast<rng_type_t>(componentData["current_rng_type"]);
        }
    };

} // namespace ECS