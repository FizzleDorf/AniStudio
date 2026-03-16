#pragma once

// sd_type constants - matching enum sd_type_t order with gaps
constexpr const char *type_method_items[] = {
	"SD_TYPE_F32",
	"SD_TYPE_F16",
	"SD_TYPE_Q4_0",
	"SD_TYPE_Q4_1",
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
	"SD_TYPE_TQ1_0",
	"SD_TYPE_TQ2_0",
	"SD_TYPE_MXFP4"
};
constexpr const int type_method_item_count = sizeof(type_method_items) / sizeof(type_method_items[0]);

// sample_method_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *sample_method_items[] = {
	"EULER",
	"EULER_A",
	"HEUN",
	"DPM2",
	"DPMPP2S_A",
	"DPMPP2M",
	"DPMPP2Mv2",
	"IPNDM",
	"IPNDM_V",
	"LCM",
	"DDIM_TRAILING",
	"TCD",
	"RES_MULTISTEP",
	"RES_2S"
};
constexpr const int sample_method_item_count = sizeof(sample_method_items) / sizeof(sample_method_items[0]);

// scheduler_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *scheduler_method_items[] = {
	"DISCRETE",
	"KARRAS",
	"EXPONENTIAL",
	"AYS",
	"GITS",
	"SGM_UNIFORM",
	"SIMPLE",
	"SMOOTHSTEP",
	"KL_OPTIMAL",
	"LCM",
	"BONG_TANGENT"
};
constexpr const int scheduler_method_item_count = sizeof(scheduler_method_items) / sizeof(scheduler_method_items[0]);

// rng_type_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *type_rng_items[] = {
	"STD_DEFAULT_RNG",
	"CUDA_RNG",
	"CPU_RNG"
};
constexpr const int type_rng_item_count = sizeof(type_rng_items) / sizeof(type_rng_items[0]);

// prediction_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *prediction_type_items[] = {
	"EPS_PRED",
	"V_PRED",
	"EDM_V_PRED",
	"FLOW_PRED",
	"FLUX_FLOW_PRED",
	"FLUX2_FLOW_PRED"
};
constexpr const int prediction_type_item_count = sizeof(prediction_type_items) / sizeof(prediction_type_items[0]);

// preview_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *preview_type_items[] = {
	"PREVIEW_NONE",
	"PREVIEW_PROJ",
	"PREVIEW_TAE",
	"PREVIEW_VAE"
};
constexpr const int preview_type_item_count = sizeof(preview_type_items) / sizeof(preview_type_items[0]);

// lora_apply_mode_t constants - matching exact enum order from stable-diffusion.h
constexpr const char *lora_apply_mode_items[] = {
	"LORA_APPLY_AUTO",
	"LORA_APPLY_IMMEDIATELY",
	"LORA_APPLY_AT_RUNTIME"
};
constexpr const int lora_apply_mode_item_count = sizeof(lora_apply_mode_items) / sizeof(lora_apply_mode_items[0]);