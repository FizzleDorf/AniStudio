#pragma once

#include "Components.h"
#include "EntityManager.hpp"
#include "stable-diffusion.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>

using namespace ECS;

namespace ECS {

struct Image {
    unsigned char *data;
    int width;
    int height;
    int channel;
};

struct InferenceComponent : public ECS::BaseComponent {

    bool shouldInference = true;

    const char *model_path = "D:/Stable Diffusion/models/checkpoints/based66_v30.safetensors";
    const char *txxl_path = "";
    const char *clip_l_path = "";
    const char *clip_g_path = "";
    const char *diffusion_model_path = "";
    const char *vae_path = "D:/Stable Diffusion/models/vae/vae-ft-mse-840000-ema-pruned.ckpt";
    const char *control_net_path = "";
    const char *lora_model_dir = "";
    const char *embed_dir = "";
    const char *taesd_path = "";
    const char *stacked_id_embed_dir_c_str = "";

    std::string outputPath = "./AniStudio.png";

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

};
} // namespace ECS
