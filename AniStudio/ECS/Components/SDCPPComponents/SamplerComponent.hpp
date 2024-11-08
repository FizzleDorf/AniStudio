#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

// sampler method constants
constexpr const char *sample_method_items[] = {"Euler a",  "Euler",       "Heun",  "Dpm 2",   "Dpmpp 2 a",
                                               "Dpmpp 2m", "Dpm++ 2m v2", "Ipndm", "Ipndm v", "Lcm"};
constexpr int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);

// scheduler method constants
constexpr const char *scheduler_method_items[] = {"Default", "Discrete", "Karras",     "Exponential",
                                                  "Ays",     "Gits",     "N schedules"};
constexpr int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);

// sd_type constants
constexpr const char *type_method_items[] = {
    "SD_TYPE_F32",      "SD_TYPE_F16",      "SD_TYPE_Q4_0",    "SD_TYPE_Q4_1",   "SD_TYPE_Q5_0",    "SD_TYPE_Q5_1",
    "SD_TYPE_Q8_0",     "SD_TYPE_Q8_1",     "SD_TYPE_Q2_K",    "SD_TYPE_Q3_K",   "SD_TYPE_Q4_K",    "SD_TYPE_Q5_K",
    "SD_TYPE_Q6_K",     "SD_TYPE_Q8_K",     "SD_TYPE_IQ2_XXS", "SD_TYPE_IQ2_XS", "SD_TYPE_IQ3_XXS", "SD_TYPE_IQ1_S",
    "SD_TYPE_IQ4_NL",   "SD_TYPE_IQ3_S",    "SD_TYPE_IQ2_S",   "SD_TYPE_IQ4_XS", "SD_TYPE_I8",      "SD_TYPE_I16",
    "SD_TYPE_I32",      "SD_TYPE_I64",      "SD_TYPE_F64",     "SD_TYPE_IQ1_M",  "SD_TYPE_BF16",    "SD_TYPE_Q4_0_4_4",
    "SD_TYPE_Q4_0_4_8", "SD_TYPE_Q4_0_8_8", "SD_TYPE_COUNT"};
constexpr int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);

// rng type constants
constexpr const char *type_rng_items[] = {"STD_DEFAULT_RNG", "CUDA_RNG"};
constexpr int type_rng_item_count = sizeof(type_rng_items) / sizeof(type_rng_items[0]);

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
