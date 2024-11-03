#include "../../../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <future> // Include for std::future and std::async

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
        AddComponentSignature<ModelComponent>();
        AddComponentSignature<CLipLComponent>();
        AddComponentSignature<CLipGComponent>();
        AddComponentSignature<T5XXLComponent>();
        AddComponentSignature<DiffusionModelComponent>();
        AddComponentSignature<VaeComponent>();
        AddComponentSignature<LoraComponent>();
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<ImageComponent>();
        AddComponentSignature<SamplerComponent>();
        AddComponentSignature<CFGComponent>();
        AddComponentSignature<PromptComponent>();
    }

    void Start() override {}

    void Inference(EntityID entityID) {
        if (inferenceRunning.load()) {
            std::cout << "Inference is already running; skipping this request." << std::endl;
            return;
        }
        std::cout << "Inference is started" << std::endl;
        inferenceRunning.store(true);

        // Launch asynchronous task with std::async
        inferenceFuture = std::async(std::launch::async, [this, entityID]() {
            try {
                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);

                // Using heap allocation for context and image
                sd_ctx_t *sd_context =
                    new_sd_ctx(mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(),
                               mgr.GetComponent<CLipLComponent>(entityID).encoderPath.c_str(),
                               mgr.GetComponent<CLipGComponent>(entityID).encoderPath.c_str(),
                               mgr.GetComponent<T5XXLComponent>(entityID).encoderPath.c_str(),
                               mgr.GetComponent<DiffusionModelComponent>(entityID).ckptPath.c_str(),
                               mgr.GetComponent<VaeComponent>(entityID).vaePath.c_str(),
                               "", // taesd_path
                               "", // control_net_path
                               mgr.GetComponent<LoraComponent>(entityID).loraPath.c_str(),
                               "",    // embed_dir
                               "",    // stacked_id_embed_dir
                               false, // vae_decode_only
                               mgr.GetComponent<VaeComponent>(entityID).isTiled,
                               true, // free_params_immediately
                               4,    // n_threads
                               sd_type_t::SD_TYPE_F16, rng_type_t::STD_DEFAULT_RNG, schedule_t::DEFAULT,
                               true,  // keep_clip_on_cpu
                               false, // keep_control_net_cpu
                               false  // keep_vae_on_cpu
                    );

                if (!sd_context) {
                    throw std::runtime_error(
                        "Failed to initialize Stable Diffusion context! Please check paths and parameters.");
                }
                std::cout << "Stable Diffusion context initialized successfully." << std::endl;

                // Perform image generation
                sd_image_t *image = txt2img(sd_context, mgr.GetComponent<PromptComponent>(entityID).posPrompt.c_str(),
                                            mgr.GetComponent<PromptComponent>(entityID).negPrompt.c_str(),
                                            0, // clip_skip
                                            mgr.GetComponent<CFGComponent>(entityID).cfg,
                                            1.0f, // guidance_scale
                                            mgr.GetComponent<LatentComponent>(entityID).latentWidth,
                                            mgr.GetComponent<LatentComponent>(entityID).latentHeight,
                                            mgr.GetComponent<SamplerComponent>(entityID).current_sample_method,
                                            mgr.GetComponent<SamplerComponent>(entityID).steps,
                                            31337, // seed
                                            mgr.GetComponent<LatentComponent>(entityID).batchSize,
                                            nullptr, // control_cond
                                            0.0f,    // control_strength
                                            0.0f,    // style_strength
                                            false,   // normalize_input
                                            ""       // input_id_images_path
                );

                if (!image) {
                    throw std::runtime_error("Failed to generate image! Please verify input parameters.");
                }
                std::cout << "Image generated successfully." << std::endl;

                // Clean up resources
                free_sd_ctx(sd_context);
                // Free the image here if necessary, depending on your library's API

            } catch (const std::exception &e) {
                std::cerr << "Exception in async task: " << e.what() << std::endl;
            }
            mgr.DestroyEntity(entityID);
            inferenceRunning.store(false);
        });
    }


    void Update() override {
        if (!inferenceRunning.load() && !inferenceQueue.empty()) {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::cout << "num entities: " << mgr.GetEntityCount() << std::endl;
            EntityID entityID = inferenceQueue.front();
            inferenceQueue.pop();
            std::cout << "Dequeuing entity for inference, ID: " << entityID << std::endl;
            if (mgr.HasComponent<ModelComponent>(entityID) || mgr.HasComponent<DiffusionModelComponent>(entityID)) {
                std::cout << mgr.GetComponent<ModelComponent>(entityID).modelPath << entityID << std::endl;
                Inference(entityID);
            } else {
                std::cerr << "No model path provided with Entity ID: " << entityID << std::endl;
            }
        }

        // Check if the inference task is done if it was running
        if (inferenceFuture.valid() && inferenceFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            inferenceFuture.get(); // Retrieve result and allow exceptions to be thrown if any
        }
    }

    void QueueInference(EntityID entityID) {
        std::lock_guard<std::mutex> lock(queueMutex); // Lock while modifying the queue
        inferenceQueue.push(entityID);
        std::cout << "Entity Queued for inference." << std::endl;
    }

private:
    EntityManager &mgr = ECS::EntityManager::Ref();
    std::queue<EntityID> inferenceQueue;       // Queue to hold entity IDs for inference
    std::mutex queueMutex;                     // Mutex to protect access to inferenceQueue
    std::atomic<bool> inferenceRunning{false}; // Atomic flag to track if inference is running
    std::future<void> inferenceFuture;         // Future for tracking the async inference task
};
} // namespace ECS
