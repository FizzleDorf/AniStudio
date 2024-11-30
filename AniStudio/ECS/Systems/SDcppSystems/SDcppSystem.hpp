#include "../../../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <filesystem>
#include <future>
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
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<InputImageComponent>();
    }

    void Start() override {}

    void SaveImage(const unsigned char *imageData, int width, int height, int channels) {
        // Construct the filename with a zero-padded counter
        std::stringstream filename;
        filename << "./AniStudio_" << std::setw(5) << std::setfill('0') << saveCounter++ << ".png";

        // Save the image using stbi_write_png
        if (!stbi_write_png(filename.str().c_str(), width, height, channels, imageData, width * channels)) {
            std::cerr << "Failed to save image: " << filename.str() << "\n";
        } else {
            std::cout << "Image saved successfully: " << filename.str() << "\n";
        }
    }

    void Inference(const EntityID entityID) {
        std::lock_guard<std::mutex> lock(inferenceMutex);

        if (inferenceRunning.load()) {
            std::cout << "Inference is already running; skipping this request."
                      << "\n";
            return;
        }

        std::cout << "Inference started." << std::endl;
        inferenceRunning.store(true);

        // Launch asynchronous inference task
        inferenceFuture = std::async(std::launch::async, [this, entityID]() {
            try {
                std::lock_guard<std::mutex> mgrLock(mgrMutex);

                // Initialize Stable Diffusion context
                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);

                sd_ctx_t *sd_context = InitializeStableDiffusionContext(entityID);
                if (!sd_context) {
                    throw std::runtime_error("Failed to initialize Stable Diffusion context!");
                }

                // Generate image
                sd_image_t *image = GenerateImage(sd_context, entityID);
                if (!image) {
                    throw std::runtime_error("Failed to generate image!");
                }

                // Save image to file
                SaveImage(image->data, image->width, image->height, image->channel);

                // Clean up
                delete image;
                free_sd_ctx(sd_context);
                std::cout << "Inference completed successfully."
                          << "\n";

            } catch (const std::exception &e) {
                std::cerr << "Exception during inference: " << e.what() << std::endl;
            }
            inferenceRunning.store(false);
        });
    }

    // Update called in the Engine Update Loop
    void Update() override {
        std::lock_guard<std::mutex> lock(queueMutex);

        // Process inference queue
        if (!inferenceRunning.load() && !inferenceQueue.empty()) {
            EntityID entityID = inferenceQueue.front();
            inferenceQueue.pop();

            if (ArePathsValid(entityID)) {
                Inference(entityID);
            } else {
                std::cerr << "Invalid paths for entity " << entityID << std::endl;
            }
        }
        // Handle completed inference task
        if (inferenceFuture.valid() && inferenceFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            inferenceFuture.get(); // Allow exceptions to propagate
        }
    }

    // Queues an Entity to be processed
    void QueueInference(EntityID entityID) {
        inferenceQueue.push(entityID);
        std::cout << "Entity queued for inference." << std::endl;
    }

private:
    EntityManager &mgr = ECS::EntityManager::Ref();
    std::queue<EntityID> inferenceQueue;
    std::mutex queueMutex;
    std::mutex inferenceMutex;
    std::mutex mgrMutex;
    std::atomic<bool> inferenceRunning{false};
    std::future<void> inferenceFuture;
    int saveCounter = 0;
    // Utility method to initialize Stable Diffusion text to image context
    sd_ctx_t *InitializeStableDiffusionContext(const EntityID entityID) {
        return new_sd_ctx(mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<CLipLComponent>(entityID).encoderPath.c_str(),
                          mgr.GetComponent<CLipGComponent>(entityID).encoderPath.c_str(),
                          mgr.GetComponent<T5XXLComponent>(entityID).encoderPath.c_str(),
                          mgr.GetComponent<DiffusionModelComponent>(entityID).ckptPath.c_str(),
                          mgr.GetComponent<VaeComponent>(entityID).vaePath.c_str(),
                          mgr.GetComponent<TaesdComponent>(entityID).taesdPath.c_str(), "",
                          mgr.GetComponent<LoraComponent>(entityID).loraPath.c_str(),
                          mgr.GetComponent<EmbeddingComponent>(entityID).embedPath.c_str(), "",
                          mgr.GetComponent<VaeComponent>(entityID).vae_decode_only,
                          mgr.GetComponent<VaeComponent>(entityID).isTiled,
                          mgr.GetComponent<SamplerComponent>(entityID).free_params_immediately,
                          mgr.GetComponent<SamplerComponent>(entityID).n_threads,
                          mgr.GetComponent<SamplerComponent>(entityID).current_type_method,
                          mgr.GetComponent<SamplerComponent>(entityID).current_rng_type,
                          mgr.GetComponent<SamplerComponent>(entityID).current_scheduler_method, true, false,
                          mgr.GetComponent<VaeComponent>(entityID).keep_vae_on_cpu, false);
    }

    // Utility method to generate text to image using Stable Diffusion
    sd_image_t *GenerateImage(sd_ctx_t *context, const EntityID entityID) {
        return txt2img(context, mgr.GetComponent<PromptComponent>(entityID).posPrompt.c_str(),
                       mgr.GetComponent<PromptComponent>(entityID).negPrompt.c_str(), 0,
                       mgr.GetComponent<CFGComponent>(entityID).cfg, 
                       mgr.GetComponent<CFGComponent>(entityID).guidance,
                       mgr.GetComponent<LatentComponent>(entityID).latentWidth,
                       mgr.GetComponent<LatentComponent>(entityID).latentHeight,
                       mgr.GetComponent<SamplerComponent>(entityID).current_sample_method,
                       mgr.GetComponent<SamplerComponent>(entityID).steps,
                       mgr.GetComponent<SamplerComponent>(entityID).seed,
                       mgr.GetComponent<LatentComponent>(entityID).batchSize, nullptr, 0.0f, 0.0f, false, "",
                       mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers,
                       mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers_count,
                       mgr.GetComponent<LayerSkipComponent>(entityID).slg_scale,
                       mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_start,
                       mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_end
            );
    }

    // Utility to validate component paths
    bool ArePathsValid(const EntityID entityID) {
        return std::filesystem::exists(mgr.GetComponent<ModelComponent>(entityID).modelPath) ||
               std::filesystem::exists(mgr.GetComponent<DiffusionModelComponent>(entityID).ckptPath);
    }
};

} // namespace ECS
