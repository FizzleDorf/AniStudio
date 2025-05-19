/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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
constexpr const char *sample_method_items[] = {"Euler a",  "Euler", "Heun",  "Dpm 2", "Dpm++ 2 a",
                                               "Dpm++ 2m", "Dpm++ 2m v2", "Ipndm", "Ipndm v", "Lcm", "Ddim Trailing", "TCD"};
constexpr const int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);

// scheduler method constants
constexpr const char *scheduler_method_items[] = {"Default", "Discrete", "Karras", "Exponential", "Ays", "Gits"};
constexpr const int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);

// rng type constants
constexpr const char *type_rng_items[] = {"STD_DEFAULT_RNG", "CUDA_RNG"};
constexpr const int type_rng_item_count = sizeof(type_rng_items) / sizeof(type_rng_items[0]);