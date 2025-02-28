#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "Constants.hpp"
#include <string>

namespace ECS {

struct SamplerComponent : public ECS::BaseComponent {
    SamplerComponent() { compName = "Sampler"; }
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

    SamplerComponent &operator=(const SamplerComponent &other) {
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
        return {{compName,
                 {{"seed", seed},
                  {"steps", steps},
                  {"cfg", cfg},
                  {"denoise", denoise},
                  {"n_threads", n_threads},
                  {"free_params_immediately", free_params_immediately},
                  {"current_sample_method", static_cast<int>(current_sample_method)},
                  {"current_scheduler_method", static_cast<int>(current_scheduler_method)},
                  {"current_type_method", static_cast<int>(current_type_method)},
                  {"current_rng_type", static_cast<int>(current_rng_type)}}}};
    }

    void Deserialize(const nlohmann::json& j) {
        
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
