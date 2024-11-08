#include "../../../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <filesystem>
#include <future> // Include for std::future and std::async
#include <vector>

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
        AddComponentSignature<DiffusionModelComponent>();
    }

    void Start() override {}

    void Inference(const EntityID entityID) {
        std::lock_guard<std::mutex> lock(inferenceMutex); 
        if (inferenceRunning.load()) {
            std::cout << "Inference is already running; skipping this request." << std::endl;
            return;
        }
        std::cout << "Inference is started" << std::endl;
        inferenceRunning.store(true);

        // Launch asynchronous task with std::async
        inferenceFuture = std::async(std::launch::async, [this, entityID]() {
            try {
                std::lock_guard<std::mutex> lock(mgrMutex);
                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);

                // Using heap allocation for context and image
                sd_ctx_t *sd_context = new_sd_ctx(mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(),
                                                  mgr.GetComponent<CLipLComponent>(entityID).encoderPath.c_str(),
                                                  mgr.GetComponent<CLipGComponent>(entityID).encoderPath.c_str(),
                                                  mgr.GetComponent<T5XXLComponent>(entityID).encoderPath.c_str(),
                                                  mgr.GetComponent<DiffusionModelComponent>(entityID).ckptPath.c_str(),
                                                  mgr.GetComponent<VaeComponent>(entityID).vaePath.c_str(),
                                                  mgr.GetComponent<TaesdComponent>(entityID).taesdPath.c_str(),
                                                  "", // control_net_path
                                                  mgr.GetComponent<LoraComponent>(entityID).loraPath.c_str(),
                                                  mgr.GetComponent<EmbeddingComponent>(entityID).embedPath.c_str(),
                                                  "", // stacked_id_embed_dir
                                                  mgr.GetComponent<VaeComponent>(entityID).vae_decode_only,
                                                  mgr.GetComponent<VaeComponent>(entityID).isTiled,
                                                  mgr.GetComponent<SamplerComponent>(entityID).free_params_immediately,
                                                  mgr.GetComponent<SamplerComponent>(entityID).n_threads,
                                                  mgr.GetComponent<SamplerComponent>(entityID).current_type_method,
                                                  mgr.GetComponent<SamplerComponent>(entityID).current_rng_type,
                                                  mgr.GetComponent<SamplerComponent>(entityID).current_scheduler_method,
                                                  true,  // keep_clip_on_cpu
                                                  false, // keep_control_net_cpu
                                                  mgr.GetComponent<VaeComponent>(entityID).keep_vae_on_cpu);

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
                                            mgr.GetComponent<CFGComponent>(entityID).guidance,
                                            mgr.GetComponent<LatentComponent>(entityID).latentWidth,
                                            mgr.GetComponent<LatentComponent>(entityID).latentHeight,
                                            mgr.GetComponent<SamplerComponent>(entityID).current_sample_method,
                                            mgr.GetComponent<SamplerComponent>(entityID).steps,
                                            mgr.GetComponent<SamplerComponent>(entityID).seed,
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

                // Copy image data to a buffer to ensure it's available for saving
                int dataSize = image->width * image->height * image->channel;
                std::vector<unsigned char> imageData(image->data, image->data + dataSize);

                // Store the image properties
                mgr.GetComponent<ImageComponent>(entityID).imageData = image->data;
                mgr.GetComponent<ImageComponent>(entityID).width = image->width;
                mgr.GetComponent<ImageComponent>(entityID).height = image->height;
                mgr.GetComponent<ImageComponent>(entityID).channels = image->channel;

                // Debug output for verification
                std::cout << "Saving image with width: " << image->width << ", height: " << image->height
                          << ", channels: " << image->channel << std::endl;

                // Save image using stb_image
                int stride = image->width * image->channel;
                stbi_write_png("./Anistudio.png", image->width, image->height, image->channel, imageData.data(),
                               stride);

                // Clean up resources
                free_sd_ctx(sd_context);

            } catch (const std::exception &e) {
                std::cerr << "Exception in async task: " << e.what() << std::endl;
            }
            inferenceRunning.store(false);
        });
    }

    void Update() override {
        std::lock_guard<std::mutex> lock(queueMutex); // Lock while modifying the queue
        if (!inferenceRunning.load() && !inferenceQueue.empty()) {
            EntityID entityID = inferenceQueue.front();
            inferenceQueue.pop();
            if (std::filesystem::exists(mgr.GetComponent<ModelComponent>(entityID).modelPath) ||
                std::filesystem::exists(mgr.GetComponent<DiffusionModelComponent>(entityID).ckptPath)) {
                Inference(entityID);
            } else {
                mgr.DestroyEntity(entityID);
            }
        }

        // Check if the inference task is done if it was running
        if (inferenceFuture.valid() && inferenceFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            inferenceFuture.get(); // Retrieve result and allow exceptions to be thrown if any
        }
    }

    void QueueInference(EntityID entityID) {
        std::lock_guard<std::mutex> lock(queueMutex);
        inferenceQueue.push(entityID);
        std::cout << "Entity Queued for inference." << std::endl;
    }


private:
    EntityManager &mgr = ECS::EntityManager::Ref();
    std::queue<EntityID> inferenceQueue;       // Queue to hold entity IDs for inference
    std::mutex queueMutex;
    std::mutex inferenceMutex;
    std::mutex mgrMutex;
    std::atomic<bool> inferenceRunning{false}; // Atomic flag to track if inference is running
    std::future<void> inferenceFuture;         // Future for tracking the async inference task
};
} // namespace ECS
