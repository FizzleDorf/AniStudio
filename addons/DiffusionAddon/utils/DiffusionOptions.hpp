#pragma once

// sd_type constants - matching enum sd_type_t order with gaps
constexpr const char *type_method_items[] = {
	"SD_TYPE_F32",
	"SD_TYPE_F16",
	"SD_TYPE_Q4_0",
	"SD_TYPE_Q4_1",
	"SD_TYPE_Q4_2 (N/A)",
	"SD_TYPE_Q4_3 (N/A)",
	"SD_TYPE_Q5_0",
	"SD_TYPE_Q5_1",
	"SD_TYPE_Q8_0",
	"SD_TYPE_Q8_1",
	"SD_TYPE_Q2_K",
	"SD_TYPE_Q3_K",
	"SD_TYPE_Q4_K",
	"SD_TYPE_Q5_K",
	"SD_TYPE_Q6_K",
	"SD_TYPE_Q8_K",
	"SD_TYPE_IQ2_XXS",
	"SD_TYPE_IQ2_XS",
	"SD_TYPE_IQ3_XXS",
	"SD_TYPE_IQ1_S",
	"SD_TYPE_IQ4_NL",
	"SD_TYPE_IQ3_S",
	"SD_TYPE_IQ2_S",
	"SD_TYPE_IQ4_XS",
	"SD_TYPE_I8",
	"SD_TYPE_I16",
	"SD_TYPE_I32",
	"SD_TYPE_I64",
	"SD_TYPE_F64",
	"SD_TYPE_IQ1_M",
	"SD_TYPE_BF16",
	"SD_TYPE_Q4_0_4_4 (N/A)",
	"SD_TYPE_Q4_0_4_8 (N/A)",
	"SD_TYPE_Q4_0_8_8 (N/A)",
	"SD_TYPE_TQ1_0",
	"SD_TYPE_TQ2_0",
	"SD_TYPE_IQ4_NL_4_4 (N/A)",
	"SD_TYPE_IQ4_NL_4_8 (N/A)",
	"SD_TYPE_IQ4_NL_8_8 (N/A)",
	"SD_TYPE_MXFP4"
};

constexpr const int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);

// sampler method constants - in exact enum order from C++ header
constexpr const char *sample_method_items[] = {
	"DEFAULT", "EULER", "HEUN", "DPM2", "DPMPP2S_A",
	"DPMPP2M", "DPMPP2Mv2", "IPNDM", "IPNDM_V", "LCM",
	"DDIM_TRAILING", "TCD", "EULER_A"
};
constexpr const int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);

// scheduler method constants - matching enum scheduler_t order
constexpr const char *scheduler_method_items[] = {
	"DEFAULT", "DISCRETE", "KARRAS", "EXPONENTIAL", "AYS", "GITS", "SGM_UNIFORM", "SIMPLE", "SMOOTHSTEP"
};
constexpr const int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);

// rng type constants - matching enum rng_type_t order
constexpr const char *type_rng_items[] = { "STD_DEFAULT_RNG", "CUDA_RNG" };
constexpr const int type_rng_item_count = sizeof(type_rng_items) / sizeof(type_rng_items[0]);

// prediction type constants - matching enum prediction_t order
constexpr const char *prediction_type_items[] = {
	"EPS_PRED", "V_PRED", "EDM_V_PRED", "SD3_FLOW_PRED", "FLUX_FLOW_PRED"
};
constexpr const int prediction_type_item_count = sizeof(prediction_type_items) / sizeof(prediction_type_items[0]);