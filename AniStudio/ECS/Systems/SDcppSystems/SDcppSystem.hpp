#pragma once
#include "ECS.hpp"
#include "SDCPPComponents.h"
#include "ImageComponents/ImageComponent.hpp"
#include "stable-diffusion.h"

using namespace ECS;

class SDCCPSystem : public BaseSystem {
public:
    SDCCPSystem() {
        mgr.AddComponentSignature<PromptComponent>();
        mgr.AddComponentSignature<CFGComponent>();
        mgr.AddComponentSignature<SamplerComponent>();
        mgr.AddComponentSignature<DiffusionModelComponent>();
        mgr.AddComponentSignature<VaeComponent>();
        mgr.AddComponentSignature<EncoderComponent>();
        mgr.AddComponentSignature<ControlnetComponent>();
        mgr.AddComponentSignature<EsrganComponent>();
        mgr.AddComponentSignature<LatentComponent>();
        mgr.AddComponentSignature<ImageComponent>();
    }

    void Inference(EntityManager &mgr, EntityID entityID) {
        // Get components
        auto &prompt = mgr.GetComponent<PromptComponent>(entityID);
        auto &cfg = mgr.GetComponent<CFGComponent>(entityID);
        auto &sampler = mgr.GetComponent<SamplerComponent>(entityID);
        auto &model = mgr.GetComponent<DiffusionModelComponent>(entityID);
        auto &output = mgr.GetComponent<ImageComponent>(entityID);

        // Prepare model paths (ensure you have these paths correctly set)
        const char *model_path = *model.model_path; 
        const char *clip_l_path = "path/to/clip_l";    
        const char *t5xxl_path = "path/to/t5xxl";
        const char *diffusion_model_path = "path/to/diffusion_model";
        const char *vae_path = "path/to/vae";
        const char *control_net_path = "path/to/control_net";
        const char *lora_model_dir = "path/to/lora";
        const char *embed_dir = "path/to/embed";

        // Create a new context for the stable diffusion model
        sd_ctx_t *sd_ctx = new_sd_ctx(model_path, clip_l_path, t5xxl_path, diffusion_model_path, vae_path, nullptr,
                                      control_net_path, lora_model_dir, embed_dir, nullptr, false, false, 4,
                                      SD_TYPE_F32, STD_DEFAULT_RNG, DEFAULT, true, true, true);

        // Call the txt2img function (or img2img depending on your use case)
        sd_image_t *image = txt2img(sd_ctx, prompt.GetPrompt().c_str(), nullptr, 0, cfg.GetCFG(), 1.0f, 512, 512,
                                    EULER_A, 50, 0, 1, nullptr, 0.0f, 0.0f, true, nullptr);

        // Process the generated image (e.g., save it)
        output.SetImageData(image->data, image->width, image->height, image->channel);

        // Free the image and context
        free(image);
        free_sd_ctx(sd_ctx);
    }
};