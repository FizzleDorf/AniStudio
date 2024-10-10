//#pragma once
//#include "ECS.hpp"
//#include "SDCPPComponents.h"
//#include "ImageComponents/ImageIOComponent.hpp"
//#include "stable-diffusion.h"
//
//using namespace ECS;
//
//class SDCCPSystem : public BaseSystem {
//public:
//    SDCCPSystem() {
//        AddComponentSignature<PromptComponent>();
//        AddComponentSignature<CFGComponent>();
//        AddComponentSignature<SamplerComponent>();
//        AddComponentSignature<DiffusionModelComponent>();
//        AddComponentSignature<ImageIOComponent>();
//    }
//
//    void Inference(EntityManager &entityManager, EntityID entityID) {
//        // Get components
//        auto &prompt = entityManager.GetComponent<PromptComponent>(entityID);
//        auto &cfg = entityManager.GetComponent<CFGComponent>(entityID);
//        auto &sampler = entityManager.GetComponent<SamplerComponent>(entityID);
//        auto &model = entityManager.GetComponent<DiffusionModelComponent>(entityID);
//        auto &output = entityManager.GetComponent<ImageIOComponent>(entityID);
//
//        // Prepare model paths (ensure you have these paths correctly set)
//        const char *model_path = model.GetModelPath(); // Add a method in your model component to get the model path
//        const char *clip_l_path = "path/to/clip_l";    // Set your paths correctly
//        const char *t5xxl_path = "path/to/t5xxl";
//        const char *diffusion_model_path = "path/to/diffusion_model";
//        const char *vae_path = "path/to/vae";
//        const char *control_net_path = "path/to/control_net";
//        const char *lora_model_dir = "path/to/lora";
//        const char *embed_dir = "path/to/embed";
//
//        // Create a new context for the stable diffusion model
//        sd_ctx_t *sd_ctx = new_sd_ctx(model_path, clip_l_path, t5xxl_path, diffusion_model_path, vae_path, nullptr,
//                                      control_net_path, lora_model_dir, embed_dir, nullptr, false, false, 4,
//                                      SD_TYPE_F32, STD_DEFAULT_RNG, DEFAULT, true, true, true);
//
//        // Call the txt2img function (or img2img depending on your use case)
//        sd_image_t *image = txt2img(sd_ctx, prompt.GetPrompt().c_str(), nullptr, 0, cfg.GetCFG(), 1.0f, 512, 512,
//                                    EULER_A, 50, 0, 1, nullptr, 0.0f, 0.0f, true, nullptr);
//
//        // Process the generated image (e.g., save it)
//        output.SetImageData(image->data, image->width, image->height, image->channel);
//
//        // Free the image and context
//        free(image);
//        free_sd_ctx(sd_ctx);
//    }
//};