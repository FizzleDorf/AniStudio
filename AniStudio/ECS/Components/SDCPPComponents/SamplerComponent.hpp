#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {
struct SamplerComponent : public ECS::BaseComponent {
    int steps = 20;
    float denoise = 1.0;

    // sampler
    static constexpr const char *sample_method_items[] = {"Euler a",  "Euler",       "Heun",  "Dpm 2",   "Dpmpp 2 a",
                                                          "Dpmpp 2m", "Dpm++ 2m v2", "Ipndm", "Ipndm v", "Lcm"};
    static constexpr int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);
    sample_method_t current_sample_method = sample_method_t::EULER;

    // scheduler
    static constexpr const char *scheduler_method_items[] = {"Default", "Discrete", "Karras",     "Exponential",
                                                             "Ays",     "Gits",     "N schedules"};
    static constexpr int scheduler_method_item_count =
        sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);
    schedule_t current_scheduler_method = schedule_t::DEFAULT;

    // SD_Types
    static constexpr const char *type_method_items[] = {
        "SD_TYPE_F32",      "SD_TYPE_F16",      "SD_TYPE_Q4_0",  "SD_TYPE_Q4_1",   "SD_TYPE_Q5_0",
        "SD_TYPE_Q5_1",     "SD_TYPE_Q8_0",     "SD_TYPE_Q8_1",  "SD_TYPE_Q2_K",   "SD_TYPE_Q3_K",
        "SD_TYPE_Q4_K",     "SD_TYPE_Q5_K",     "SD_TYPE_Q6_K",  "SD_TYPE_Q8_K",   "SD_TYPE_IQ2_XXS",
        "SD_TYPE_IQ2_XS",   "SD_TYPE_IQ3_XXS",  "SD_TYPE_IQ1_S", "SD_TYPE_IQ4_NL", "SD_TYPE_IQ3_S",
        "SD_TYPE_IQ2_S",    "SD_TYPE_IQ4_XS",   "SD_TYPE_I8",    "SD_TYPE_I16",    "SD_TYPE_I32",
        "SD_TYPE_I64",      "SD_TYPE_F64",      "SD_TYPE_IQ1_M", "SD_TYPE_BF16",   "SD_TYPE_Q4_0_4_4",
        "SD_TYPE_Q4_0_4_8", "SD_TYPE_Q4_0_8_8", "SD_TYPE_COUNT"};
    static constexpr int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);
    sd_type_t current_type_method = sd_type_t::SD_TYPE_F16;

};
} // namespace ECS
