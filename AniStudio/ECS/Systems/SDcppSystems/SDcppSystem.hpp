#pragma once
#include "ECS.h"
#include "Components.h"
#include "pch.h"
#include "InferenceQueue.hpp"
#include "stable-diffusion.h"

using namespace ECS;

class SDCPPSystem : public BaseSystem {
public:
    SDCPPSystem() {
        // Register component signatures with the system
        AddComponentSignature<PromptComponent>();
        AddComponentSignature<CFGComponent>();
        AddComponentSignature<SamplerComponent>();
        AddComponentSignature<DiffusionModelComponent>();
        AddComponentSignature<VaeComponent>();
        AddComponentSignature<EncoderComponent>();
        AddComponentSignature<ControlnetComponent>();
        AddComponentSignature<EsrganComponent>();
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<ImageComponent>();
    }

    void Inference(EntityManager *mgr, EntityID entityID) {
        // Fetch the necessary components from the entity
        
        const char *t5xxl_path = nullptr;
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
            
        CLipLComponent *clip_l = nullptr;
        if (mgr->HasComponent<CLipLComponent>(entityID)) {
            clip_l = &mgr->GetComponent<CLipLComponent>(entityID);
            clip_l_path = clip_l->encoderPath.c_str();
        }
            
        TXXLComponent *t5xxl = nullptr;
        if (mgr->HasComponent<TXXLComponent>(entityID)) {
            t5xxl = &mgr->GetComponent<TXXLComponent>(entityID);
            t5xxl_path = t5xxl->encoderPath.c_str();
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
         // Assuming model_path is std::string

        // Context for stable diffusion model
        sd_ctx_t *sd_ctx = new_sd_ctx(
            model_path, 
            clip_l_path, 
            t5xxl_path, 
            diffusion_model_path, vae_path,                         
            nullptr,         // taesd_path                         
            control_net_path, 
            lora_model_dir, 
            embed_dir,                       
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
        sd_image_t *image = txt2img(
            sd_ctx, 
            prompt->posPrompt.c_str(), 
            nullptr, 
            0, 
            cfg->cfg, 
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
            nullptr
        );

        // Save or set the generated image in ImageComponent
        output->SetImageData(image->data, image->width, image->height, image->channel);

        // Free resources
        free(image);
        free_sd_ctx(sd_ctx);
    }

    void Update() {
    }
};
