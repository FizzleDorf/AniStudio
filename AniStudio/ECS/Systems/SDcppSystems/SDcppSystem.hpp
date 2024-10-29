#pragma once

#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "../../../Engine/Engine.hpp"

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
        std::thread inferenceThread([this, entityID]() {
            sd_set_log_callback(LogCallback, nullptr);
            sd_set_progress_callback(ProgressCallback, nullptr);

            InferenceComponent *inferenceComp = &mgr.GetComponent<InferenceComponent>(entityID);
            if (!inferenceComp) {
                std::cerr << "Error: InferenceComponent is null for entity ID: " << entityID << std::endl;
                return;
            }

            sd_ctx_t *sd_context =
                new_sd_ctx("D:/Stable Diffusion/models/checkpoints/based66_v30.safetensors", // model_path
                           "",                                                               // clip_l_path
                           "",
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
                                        512, 768, // width, height
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
        });
        inferenceThread.detach();
    }
    
    void Update() override {
    if (!inferenceQueue.empty()) {
        EntityID entityID = inferenceQueue.front();
        inferenceQueue.pop();
        std::cout << "Dequeuing entity for inference, ID: " << entityID << std::endl;
        Inference(entityID);
    }
}

    void QueueInference(EntityID entityID) {
        std::cout << "Entity Queued for inference." << std::endl;
        inferenceQueue.push(entityID);
    }

private:
    EntityManager &mgr = ECS::EntityManager::Ref();
    std::queue<EntityID> inferenceQueue; // Queue to hold entity IDs for inference
};
} // namespace ECS
