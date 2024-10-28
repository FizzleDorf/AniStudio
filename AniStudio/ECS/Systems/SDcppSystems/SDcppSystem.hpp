#pragma once
#include "../../../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"

static void LogCallback(sd_log_level_t level, const char *text, void *data) {
    switch (level) {
    case SD_LOG_DEBUG:
        std::cout << "[DEBUG]: " << text;
        break;
    case SD_LOG_INFO:
        std::cout << "[INFO]: " << text;
        break;
    case SD_LOG_WARN:
        std::cout << "[WARNING]: " << text;
        break;
    case SD_LOG_ERROR:
        std::cerr << "[ERROR]: " << text;
        break;
    default:
        std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
        break;
    }
}

static void ProgressCallback(int step, int steps, float time, void *data) {
    std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
}

namespace ECS {
class SDCPPSystem : public BaseSystem {
public:
    SDCPPSystem() {
        AddComponentSignature<InferenceComponent>();
        // AddComponentSignature<ImageComponent>();
    }

    void Start() override {}

    void Inference(EntityID entityID) {
        sd_set_log_callback(LogCallback, nullptr);
        sd_set_progress_callback(ProgressCallback, nullptr);

        inferenceComp = &mgr.GetComponent<InferenceComponent>(entityID);
        if (!inferenceComp) {
            std::cerr << "Error: InferenceComponent is null for entity ID: " << entityID << std::endl;
            return;
        }

        // Log parameter values to diagnose potential issues
        std::cout << "Starting inference" << std::endl;
        /*std::cout << "  Model path: " << inferenceComp->model_path << std::endl;
        std::cout << "  CLIP path: " << inferenceComp->clip_l_path << std::endl;
        std::cout << "  T5XXL path: " << inferenceComp->txxl_path << std::endl;
        std::cout << "  Diffusion model path: " << inferenceComp->diffusion_model_path << std::endl;
        std::cout << "  VAE path: " << inferenceComp->vae_path << std::endl;
        std::cout << "  Control Net path: " << inferenceComp->control_net_path << std::endl;
        std::cout << "  LORA model dir: " << inferenceComp->lora_model_dir << std::endl;
        std::cout << "  Embed dir: " << inferenceComp->embed_dir << std::endl;
        std::cout << "  ID Embed dir: " << inferenceComp->stacked_id_embed_dir_c_str << std::endl;
        std::cout << "  Threads: " << inferenceComp->n_threads << std::endl;
        std::cout << "  Width type: " << inferenceComp->wtype << std::endl;
        std::cout << "  RNG type: " << inferenceComp->rng_type << std::endl;
        std::cout << "  Schedule: " << inferenceComp->schedule << std::endl;
        std::cout << "  Keep CLIP on CPU: " << inferenceComp->keep_clip_on_cpu << std::endl;
        std::cout << "  Keep ControlNet on CPU: " << inferenceComp->keep_control_net_cpu << std::endl;
        std::cout << "  Keep VAE on CPU: " << inferenceComp->keep_vae_on_cpu << std::endl;*/

        // Continue with the Stable Diffusion context creation and inference process
        sd_ctx_t *sd_context =
            new_sd_ctx("D:/Stable Diffusion/models/checkpoints/based66_v30.safetensors", // model_path
                                      "",                          // clip_l_path
                                      "",                          // t5xxl_path
                                      "",                          // diffusion_model_path
                                      "",                          // vae_path
                                      "",                          // taesd_path
                                      "",                          // control_net_path_c_str
                                      "",                          // lora_model_dir
                                      "",                          // embed_dir_c_str
                                      "",                          // stacked_id_embed_dir_c_str
                                      true,                        // vae_decode_only
                                      false,                       // vae_tiling
                                      true,                        // free_params_immediately
                                      12,                          // n_threads
                                      sd_type_t::SD_TYPE_F16,      // wtype
                                      rng_type_t::STD_DEFAULT_RNG, // rng_type
                                      schedule_t::DEFAULT,         // schedule_t
                                      true,                        // keep_clip_on_cpu
                                      false,                       // keep_control_net_cpu
                                      false                        // keep_vae_on_cpu
        );
        sd_get_system_info();
        // Call the txt2img function
        sd_image_t *image = txt2img(sd_context, "1girl",
                                    "",         // negativePrompt
                                    0,          // clip_skip
                                    1.0f,       // cfg_scale
                                    1.0f,       // guidance
                                    1024, 1024, // width, height
                                    EULER,      // sample_method
                                    20,         // sample_steps
                                    31337,      // seed
                                    1,          // batch_count
                                    nullptr,    // control_cond
                                    0.0f,       // control_strength
                                    0.0f,       // style_strength
                                    false,      // normalize_input
                                    ""          // input_id_images_path
        );
        if (!sd_context) {
            std::cerr << "Failed to initialize Stable Diffusion context! Please check paths and parameters."
                      << std::endl;
            return;
        }

        std::cout << "Stable Diffusion context initialized successfully." << std::endl;

        // Perform inference as needed...
        free_sd_ctx(sd_context); // Clean up resources
    }

    void Update() override {
        // Get all entities from the EntityManager
        const auto &entitySignatures = mgr.GetEntitiesSignatures();
        for (const auto &[entityID, signature] : entitySignatures) {
            // Check if the entity has an InferenceComponent
            if (mgr.HasComponent<InferenceComponent>(entityID)) {
                inferenceComp = &mgr.GetComponent<InferenceComponent>(entityID);

                if (inferenceComp->shouldInference) {
                    Inference(entityID);
                    inferenceComp->shouldInference = false; // Reset the flag after processing
                }
            }
        }
    }

private:
    // Private member variables
    InferenceComponent *inferenceComp = nullptr;
    EntityManager &mgr = ECS::EntityManager::Ref();
};
} // namespace ECS
