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

    void SaveImage(const std::string &outputPath, Image *image) {
        // Save the image to the output path in PNG format
        if (stbi_write_png(outputPath.c_str(), image->width, image->height, image->channel, image->data,
                           image->width * image->channel)) {
            std::cout << "Image saved to " << outputPath << std::endl;
        } else {
            std::cerr << "Failed to save image to " << outputPath << std::endl;
        }
    }

    void TxtToImgInference() {
        std::cout << "Txt2Img Inference Started " << outputPath << std::endl;
        // Prepare model paths (ensure paths are correctly set)
        sd_ctx_t *sd_ctx = new_sd_ctx(
            model_path,
            clip_l_path,
            txxl_path,
            diffusion_model_path,
            vae_path,
            taesd_path,
            control_net_path, 
            lora_model_dir, 
            embed_dir, 
            stacked_id_embed_dir_c_str,
            vae_decode_only, 
            vae_tiling, 
            free_params_immediately, 
            n_threads, 
            wtype, 
            rng_type,
            schedule, 
            keep_clip_on_cpu, 
            keep_control_net_cpu, 
            keep_vae_on_cpu
        );

        // Txt2img function
        sd_image_t *image = txt2img(
            sd_ctx, 
            "1girl", 
            "", 
            0, 
            7.0f, 
            1.0f, 
            512, 
            512, 
            EULER, 
            20,
            0, 
            1, 
            nullptr, 
            0.0f, 
            0.0f, 
            true, 
            ""
        );

        // Save or set the generated image in ImageComponent
        Image output;
        output.width = image->width;
        output.height = image->height;
        output.channel = image->channel;
        output.data = image->data;

        // Free resources
        free(image);
        free_sd_ctx(sd_ctx);

        SaveImage(outputPath, &output);
    }
};
} // namespace ECS
