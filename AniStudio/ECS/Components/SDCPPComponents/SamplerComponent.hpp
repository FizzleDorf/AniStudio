#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

struct SamplerComponent : public ECS::BaseComponent {
    int steps = 20;
    float denoise = 1.0;
    int seed = 31337;
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
};

struct CFGComponent : public ECS::BaseComponent {
    float cfg = 7.0;
    float guidance = 2.0f;

    CFGComponent &operator=(const CFGComponent &other) {
        if (this != &other) { // Self-assignment check
            cfg = other.cfg;
            guidance = other.guidance;
        }
        return *this;
    }
};

} // namespace ECS
