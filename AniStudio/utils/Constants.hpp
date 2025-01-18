#pragma once

// sd_type constants
constexpr const char *type_method_items[] = {
    "SD_TYPE_F32",        "SD_TYPE_F16",      "SD_TYPE_Q4_0",     "SD_TYPE_Q4_1",     "SD_TYPE_Q4_2 (N/A)",
    "SD_TYPE_Q4_3 (N/A)", "SD_TYPE_Q5_0",     "SD_TYPE_Q5_1",     "SD_TYPE_Q8_0",     "SD_TYPE_Q8_1",
    "SD_TYPE_Q2_K",       "SD_TYPE_Q3_K",     "SD_TYPE_Q4_K",     "SD_TYPE_Q5_K",     "SD_TYPE_Q6_K",
    "SD_TYPE_Q8_K",       "SD_TYPE_IQ2_XXS",  "SD_TYPE_IQ2_XS",   "SD_TYPE_IQ3_XXS",  "SD_TYPE_IQ1_S",
    "SD_TYPE_IQ4_NL",     "SD_TYPE_IQ3_S",    "SD_TYPE_IQ2_S",    "SD_TYPE_IQ4_XS",   "SD_TYPE_I8",
    "SD_TYPE_I16",        "SD_TYPE_I32",      "SD_TYPE_I64",      "SD_TYPE_F64",      "SD_TYPE_IQ1_M",
    "SD_TYPE_BF16",       "SD_TYPE_Q4_0_4_4", "SD_TYPE_Q4_0_4_8", "SD_TYPE_Q4_0_8_8", "SD_TYPE_TQ1_0",
    "SD_TYPE_TQ2_0"};

constexpr const int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);

// sampler method constants
constexpr const char *sample_method_items[] = {"Euler a",  "Euler",       "Heun",  "Dpm 2",   "Dpmpp 2 a",
                                               "Dpmpp 2m", "Dpm++ 2m v2", "Ipndm", "Ipndm v", "Lcm"};
constexpr const int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);

// scheduler method constants
constexpr const char *scheduler_method_items[] = {"Default", "Discrete", "Karras", "Exponential", "Ays", "Gits"};
constexpr const int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);

// rng type constants
constexpr const char *type_rng_items[] = {"STD_DEFAULT_RNG", "CUDA_RNG"};
constexpr const int type_rng_item_count = sizeof(type_rng_items) / sizeof(type_rng_items[0]);