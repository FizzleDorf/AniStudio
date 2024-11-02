#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

struct InferenceComponent : public ECS::BaseComponent {
    // SD Context Params
    std::string model_path = "D:/Stable Diffusion/models/checkpoints/based66_v30.safetensors";
    std::string txxl_path = "";
    std::string clip_l_path = "";
    std::string clip_g_path = "";
    std::string diffusion_model_path = "";
    std::string vae_path = "";
    std::string control_net_path = "";
    std::string lora_model_dir = "";
    std::string embed_dir = "";
    std::string taesd_path = "";
    std::string stacked_id_embed_dir = "";
    bool vae_decode_only = false;
    bool vae_tiling = false;
    bool free_params_immediately = true;
    int n_threads = 4;
    sd_type_t wtype = sd_type_t::SD_TYPE_F16;
    rng_type_t rng_type = rng_type_t::STD_DEFAULT_RNG;
    schedule_t schedule = schedule_t::DEFAULT;
    bool keep_clip_on_cpu = true;
    bool keep_control_net_cpu = false;
    bool keep_vae_on_cpu = false;

    // Image Inference
    std::string pos_prompt = "1girl";
    std::string neg_prompt = "";
    int clip_skip = 0;
    float cfg_scale = 1.0f;
    float guidance_scale = 1.0f;
    int width = 512;
    int height = 768;
    sample_method_t sample_method = EULER;
    int sample_steps = 20;
    int seed = 31337;
    int batch_count = 1;
    // void *control_cond = nullptr;
    float control_strength = 0.0f;
    float style_strength = 0.0f;
    bool normalize_input = false;
    std::string input_id_images_path = "";

    InferenceComponent() = default;
};

} // namespace ECS
