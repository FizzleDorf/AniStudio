#pragma once

#include "Components.h"
#include "EntityManager.hpp"
#include "stable-diffusion.h"
#include "stb_image.h"

namespace ECS {
struct InferenceComponent : public ECS::BaseComponent {
    bool shouldInference = false;

    void Inference(EntityManager *mgr, EntityID entityID) {
        const char *txxl_path = nullptr;
        const char *clip_l_path = nullptr;
        const char *clip_g_path = nullptr;
        const char *diffusion_model_path = nullptr;
        const char *vae_path = nullptr;
        const char *control_net_path = nullptr;
        const char *lora_model_dir = nullptr;
        const char *embed_dir = nullptr;
        const char *model_path = nullptr;

        PromptComponent *prompt = nullptr;
        if (mgr->HasComponent<PromptComponent>(entityID)) {
            prompt = &mgr->GetComponent<PromptComponent>(entityID);
        }

        ImageComponent *output = nullptr;
        if (mgr->HasComponent<ImageComponent>(entityID)) {
            output = &mgr->GetComponent<ImageComponent>(entityID);
        }

        CLipGComponent *clip_g = nullptr;
        if (mgr->HasComponent<CLipGComponent>(entityID)) {
            clip_g = &mgr->GetComponent<CLipGComponent>(entityID);
            clip_g_path = clip_g->encoderPath.c_str();
        }

        CLipLComponent *clip_l = nullptr; // Corrected typo
        if (mgr->HasComponent<CLipLComponent>(entityID)) {
            clip_l = &mgr->GetComponent<CLipLComponent>(entityID);
            clip_l_path = clip_l->encoderPath.c_str();
        }

        T5XXLComponent *txxl = nullptr;
        if (mgr->HasComponent<T5XXLComponent>(entityID)) {
            txxl = &mgr->GetComponent<T5XXLComponent>(entityID);
            txxl_path = txxl->encoderPath.c_str();
        }

        CFGComponent *cfg = nullptr;
        if (mgr->HasComponent<CFGComponent>(entityID)) {
            cfg = &mgr->GetComponent<CFGComponent>(entityID);
        }

        SamplerComponent *sampler = nullptr;
        if (mgr->HasComponent<SamplerComponent>(entityID)) {
            sampler = &mgr->GetComponent<SamplerComponent>(entityID);
        }

        DiffusionModelComponent *model = nullptr;
        if (mgr->HasComponent<DiffusionModelComponent>(entityID)) {
            model = &mgr->GetComponent<DiffusionModelComponent>(entityID);
            model_path = model->ckptPath.c_str();
        }

        // Prepare model paths (ensure paths are correctly set)
        sd_ctx_t *sd_ctx = new_sd_ctx(model_path, clip_l_path, txxl_path, diffusion_model_path, vae_path,
                                      nullptr, // taesd_path
                                      control_net_path, lora_model_dir, embed_dir,
                                      nullptr,         // stacked_id_embed_dir_c_str
                                      false,           // vae_decode_only
                                      false,           // vae_tiling
                                      false,           // free_params_immediately
                                      4,               // n_threads
                                      SD_TYPE_F32,     // wtype
                                      STD_DEFAULT_RNG, // rng_type
                                      DEFAULT,         // schedule
                                      true,            // keep_clip_on_cpu
                                      true,            // keep_control_net_cpu
                                      true             // keep_vae_on_cpu
        );

        // Txt2img function
        sd_image_t *image = txt2img(sd_ctx, prompt->posPrompt.c_str(), nullptr, 0, cfg->cfg, 1.0f, 512, 512, EULER, 20,
                                    0, 1, nullptr, 0.0f, 0.0f, true, nullptr);

        // Save or set the generated image in ImageComponent
        if (output) {
            output->SetImageData(image->data, image->width, image->height, image->channel);
        }

        // Free resources
        free(image);
        free_sd_ctx(sd_ctx);
    }
};
} // namespace ECS
