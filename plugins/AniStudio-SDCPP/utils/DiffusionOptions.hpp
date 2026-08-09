// DiffusionOptions.hpp
#pragma once

#include "stable-diffusion.h"
#include <unordered_map>
#include <string>
#include <vector>

inline const std::unordered_map<std::string, int>& get_sample_method_map() {
    static const std::unordered_map<std::string, int> map = {
        {"EULER", EULER_SAMPLE_METHOD},
        {"EULER_A", EULER_A_SAMPLE_METHOD},
        {"HEUN", HEUN_SAMPLE_METHOD},
        {"DPM2", DPM2_SAMPLE_METHOD},
        {"DPMPP2S_A", DPMPP2S_A_SAMPLE_METHOD},
        {"DPMPP2M", DPMPP2M_SAMPLE_METHOD},
        {"DPMPP2Mv2", DPMPP2Mv2_SAMPLE_METHOD},
        {"IPNDM", IPNDM_SAMPLE_METHOD},
        {"IPNDM_V", IPNDM_V_SAMPLE_METHOD},
        {"LCM", LCM_SAMPLE_METHOD},
        {"DDIM_TRAILING", DDIM_TRAILING_SAMPLE_METHOD},
        {"TCD", TCD_SAMPLE_METHOD},
        {"RES_MULTISTEP", RES_MULTISTEP_SAMPLE_METHOD},
        {"RES_2S", RES_2S_SAMPLE_METHOD},
        {"ER_SDE", ER_SDE_SAMPLE_METHOD},
        {"EULER_CFG_PP", EULER_CFG_PP_SAMPLE_METHOD},
        {"EULER_A_CFG_PP", EULER_A_CFG_PP_SAMPLE_METHOD},
        {"EULER_GE", EULER_GE_SAMPLE_METHOD},
        {"DPMPP2M_SDE", DPMPP2M_SDE_SAMPLE_METHOD},
        {"DPMPP2M_SDE_BT", DPMPP2M_SDE_BT_SAMPLE_METHOD}
    };
    return map;
}

inline const std::vector<std::string>& get_sample_method_names() {
    static const std::vector<std::string> names = {
        "EULER", "EULER_A", "HEUN", "DPM2", "DPMPP2S_A", "DPMPP2M",
        "DPMPP2Mv2", "IPNDM", "IPNDM_V", "LCM", "DDIM_TRAILING",
        "TCD", "RES_MULTISTEP", "RES_2S", "ER_SDE", "EULER_CFG_PP",
        "EULER_A_CFG_PP", "EULER_GE", "DPMPP2M_SDE", "DPMPP2M_SDE_BT"
    };
    return names;
}

inline int sample_method_from_name(const std::string& name) {
    auto it = get_sample_method_map().find(name);
    return (it != get_sample_method_map().end()) ? it->second : EULER_SAMPLE_METHOD;
}

inline const std::unordered_map<std::string, int>& get_scheduler_map() {
    static const std::unordered_map<std::string, int> map = {
        {"DISCRETE", DISCRETE_SCHEDULER},
        {"KARRAS", KARRAS_SCHEDULER},
        {"EXPONENTIAL", EXPONENTIAL_SCHEDULER},
        {"AYS", AYS_SCHEDULER},
        {"GITS", GITS_SCHEDULER},
        {"SGM_UNIFORM", SGM_UNIFORM_SCHEDULER},
        {"SIMPLE", SIMPLE_SCHEDULER},
        {"SMOOTHSTEP", SMOOTHSTEP_SCHEDULER},
        {"KL_OPTIMAL", KL_OPTIMAL_SCHEDULER},
        {"LCM", LCM_SCHEDULER},
        {"BONG_TANGENT", BONG_TANGENT_SCHEDULER},
        {"LTX2", LTX2_SCHEDULER},
        {"LOGIT_NORMAL", LOGIT_NORMAL_SCHEDULER},
        {"FLUX2", FLUX2_SCHEDULER},
        {"FLUX", FLUX_SCHEDULER},
        {"BETA", BETA_SCHEDULER}
    };
    return map;
}

inline const std::vector<std::string>& get_scheduler_names() {
    static const std::vector<std::string> names = {
        "DISCRETE", "KARRAS", "EXPONENTIAL", "AYS", "GITS",
        "SGM_UNIFORM", "SIMPLE", "SMOOTHSTEP", "KL_OPTIMAL",
        "LCM", "BONG_TANGENT", "LTX2", "LOGIT_NORMAL",
        "FLUX2", "FLUX", "BETA"
    };
    return names;
}

inline int scheduler_from_name(const std::string& name) {
    auto it = get_scheduler_map().find(name);
    return (it != get_scheduler_map().end()) ? it->second : DISCRETE_SCHEDULER;
}

inline const std::unordered_map<std::string, int>& get_type_method_map() {
    static const std::unordered_map<std::string, int> map = {
        {"F32", SD_TYPE_F32},
        {"F16", SD_TYPE_F16},
        {"Q4_0", SD_TYPE_Q4_0},
        {"Q4_1", SD_TYPE_Q4_1},
        {"Q5_0", SD_TYPE_Q5_0},
        {"Q5_1", SD_TYPE_Q5_1},
        {"Q8_0", SD_TYPE_Q8_0},
        {"Q8_1", SD_TYPE_Q8_1},
        {"Q2_K", SD_TYPE_Q2_K},
        {"Q3_K", SD_TYPE_Q3_K},
        {"Q4_K", SD_TYPE_Q4_K},
        {"Q5_K", SD_TYPE_Q5_K},
        {"Q6_K", SD_TYPE_Q6_K},
        {"Q8_K", SD_TYPE_Q8_K},
        {"IQ2_XXS", SD_TYPE_IQ2_XXS},
        {"IQ2_XS", SD_TYPE_IQ2_XS},
        {"IQ3_XXS", SD_TYPE_IQ3_XXS},
        {"IQ1_S", SD_TYPE_IQ1_S},
        {"IQ4_NL", SD_TYPE_IQ4_NL},
        {"IQ3_S", SD_TYPE_IQ3_S},
        {"IQ2_S", SD_TYPE_IQ2_S},
        {"IQ4_XS", SD_TYPE_IQ4_XS},
        {"I8", SD_TYPE_I8},
        {"I16", SD_TYPE_I16},
        {"I32", SD_TYPE_I32},
        {"I64", SD_TYPE_I64},
        {"F64", SD_TYPE_F64},
        {"IQ1_M", SD_TYPE_IQ1_M},
        {"BF16", SD_TYPE_BF16},
        {"TQ1_0", SD_TYPE_TQ1_0},
        {"TQ2_0", SD_TYPE_TQ2_0},
        {"MXFP4", SD_TYPE_MXFP4},
        {"NVFP4", SD_TYPE_NVFP4},
        {"Q1_0", SD_TYPE_Q1_0}
    };
    return map;
}

inline const std::vector<std::string>& get_type_method_names() {
    static const std::vector<std::string> names = {
        "F32", "F16", "Q4_0", "Q4_1", "Q5_0", "Q5_1",
        "Q8_0", "Q8_1", "Q2_K", "Q3_K", "Q4_K", "Q5_K",
        "Q6_K", "Q8_K", "IQ2_XXS", "IQ2_XS", "IQ3_XXS",
        "IQ1_S", "IQ4_NL", "IQ3_S", "IQ2_S", "IQ4_XS",
        "I8", "I16", "I32", "I64", "F64", "IQ1_M",
        "BF16", "TQ1_0", "TQ2_0", "MXFP4", "NVFP4", "Q1_0"
    };
    return names;
}

inline int type_method_from_name(const std::string& name) {
    auto it = get_type_method_map().find(name);
    return (it != get_type_method_map().end()) ? it->second : SD_TYPE_F16;
}

inline const std::unordered_map<std::string, int>& get_rng_type_map() {
    static const std::unordered_map<std::string, int> map = {
        {"STD_DEFAULT_RNG", STD_DEFAULT_RNG},
        {"CUDA_RNG", CUDA_RNG},
        {"CPU_RNG", CPU_RNG}
    };
    return map;
}

inline const std::vector<std::string>& get_rng_type_names() {
    static const std::vector<std::string> names = {
        "STD_DEFAULT_RNG", "CUDA_RNG", "CPU_RNG"
    };
    return names;
}

inline int rng_type_from_name(const std::string& name) {
    auto it = get_rng_type_map().find(name);
    return (it != get_rng_type_map().end()) ? it->second : STD_DEFAULT_RNG;
}

inline const std::unordered_map<std::string, int>& get_prediction_type_map() {
    static const std::unordered_map<std::string, int> map = {
        {"EPS_PRED", EPS_PRED},
        {"V_PRED", V_PRED},
        {"EDM_V_PRED", EDM_V_PRED},
        {"FLOW_PRED", FLOW_PRED},
        {"FLUX_FLOW_PRED", FLUX_FLOW_PRED},
        {"SEFI_FLOW_PRED", SEFI_FLOW_PRED},
        {"MINIT2I_FLOW_PRED", MINIT2I_FLOW_PRED}
    };
    return map;
}

inline const std::vector<std::string>& get_prediction_type_names() {
    static const std::vector<std::string> names = {
        "EPS_PRED", "V_PRED", "EDM_V_PRED", "FLOW_PRED",
        "FLUX_FLOW_PRED", "SEFI_FLOW_PRED", "MINIT2I_FLOW_PRED"
    };
    return names;
}

inline int prediction_type_from_name(const std::string& name) {
    auto it = get_prediction_type_map().find(name);
    return (it != get_prediction_type_map().end()) ? it->second : EPS_PRED;
}

inline const std::unordered_map<std::string, int>& get_preview_type_map() {
    static const std::unordered_map<std::string, int> map = {
        {"PREVIEW_NONE", PREVIEW_NONE},
        {"PREVIEW_PROJ", PREVIEW_PROJ},
        {"PREVIEW_TAE", PREVIEW_TAE},
        {"PREVIEW_VAE", PREVIEW_VAE}
    };
    return map;
}

inline const std::vector<std::string>& get_preview_type_names() {
    static const std::vector<std::string> names = {
        "PREVIEW_NONE", "PREVIEW_PROJ", "PREVIEW_TAE", "PREVIEW_VAE"
    };
    return names;
}

inline int preview_type_from_name(const std::string& name) {
    auto it = get_preview_type_map().find(name);
    return (it != get_preview_type_map().end()) ? it->second : PREVIEW_NONE;
}

inline const std::unordered_map<std::string, int>& get_lora_apply_mode_map() {
    static const std::unordered_map<std::string, int> map = {
        {"LORA_APPLY_AUTO", LORA_APPLY_AUTO},
        {"LORA_APPLY_IMMEDIATELY", LORA_APPLY_IMMEDIATELY},
        {"LORA_APPLY_AT_RUNTIME", LORA_APPLY_AT_RUNTIME}
    };
    return map;
}

inline const std::vector<std::string>& get_lora_apply_mode_names() {
    static const std::vector<std::string> names = {
        "LORA_APPLY_AUTO", "LORA_APPLY_IMMEDIATELY", "LORA_APPLY_AT_RUNTIME"
    };
    return names;
}

inline int lora_apply_mode_from_name(const std::string& name) {
    auto it = get_lora_apply_mode_map().find(name);
    return (it != get_lora_apply_mode_map().end()) ? it->second : LORA_APPLY_AUTO;
}

inline const std::unordered_map<std::string, int>& get_cache_mode_map() {
    static const std::unordered_map<std::string, int> map = {
        {"DISABLED", SD_CACHE_DISABLED},
        {"EASYCACHE", SD_CACHE_EASYCACHE},
        {"UCACHE", SD_CACHE_UCACHE},
        {"DBCACHE", SD_CACHE_DBCACHE},
        {"TAYLORSEER", SD_CACHE_TAYLORSEER},
        {"CACHE_DIT", SD_CACHE_CACHE_DIT},
        {"SPECTRUM", SD_CACHE_SPECTRUM}
    };
    return map;
}

inline const std::vector<std::string>& get_cache_mode_names() {
    static const std::vector<std::string> names = {
        "DISABLED", "EASYCACHE", "UCACHE", "DBCACHE",
        "TAYLORSEER", "CACHE_DIT", "SPECTRUM"
    };
    return names;
}

inline int cache_mode_from_name(const std::string& name) {
    auto it = get_cache_mode_map().find(name);
    return (it != get_cache_mode_map().end()) ? it->second : SD_CACHE_DISABLED;
}

inline const std::unordered_map<std::string, int>& get_vae_format_map() {
    static const std::unordered_map<std::string, int> map = {
        {"AUTO", SD_VAE_FORMAT_AUTO},
        {"FLUX", SD_VAE_FORMAT_FLUX},
        {"SD3", SD_VAE_FORMAT_SD3},
        {"FLUX2", SD_VAE_FORMAT_FLUX2},
        {"WAN", SD_VAE_FORMAT_WAN}
    };
    return map;
}

inline const std::vector<std::string>& get_vae_format_names() {
    static const std::vector<std::string> names = {
        "AUTO", "FLUX", "SD3", "FLUX2", "WAN"
    };
    return names;
}

inline int vae_format_from_name(const std::string& name) {
    auto it = get_vae_format_map().find(name);
    return (it != get_vae_format_map().end()) ? it->second : SD_VAE_FORMAT_AUTO;
}