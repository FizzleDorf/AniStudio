#include "ECS.h"
#include "ImageSystem.hpp"
#include "pch.h"
#include "stable-diffusion.h"
#include <filesystem>
#include <stb_image.h>
#include <stb_image_write.h>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>

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
    SDCPPSystem() : inferenceRunning(false) {
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<InputImageComponent>();
    }

    ~SDCPPSystem() {
        // Gracefully stop the worker thread if needed
        StopWorker();
    }

    void QueueInference(EntityID entityID) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            inferenceQueue.push(entityID);
        }
        queueCondition.notify_one(); // Notify the worker thread
        std::cout << "Entity " << entityID << " queued for inference." << std::endl;
    }

    void Update(float deltaT) override { 
        if (!inferenceQueue.empty() && !stopWorker) {
            StartWorker();
        }
        
    
    }

private:
    std::queue<EntityID> inferenceQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> inferenceRunning;
    std::atomic<bool> stopWorker{false};
    std::thread workerThread;
    int saveCounter = 0;

    void StartWorker() {
        // Start a dedicated worker thread for inference
        workerThread = std::thread([this]() {
            while (!stopWorker) {
                EntityID entityID;

                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCondition.wait(lock, [this]() { return stopWorker || !inferenceQueue.empty(); });

                    if (stopWorker)
                        break; // Exit if stopping
                    if (inferenceQueue.empty())
                        continue;

                    entityID = inferenceQueue.front();
                    inferenceQueue.pop();
                }
                if (ArePathsValid(entityID)) {
                    RunInference(entityID);
                } else {
                    mgr.DestroyEntity(entityID);
                }
                // Run inference on the dequeued entity
                
            }
        });
    }

    void StopWorker() {
        stopWorker = true;
        queueCondition.notify_all(); // Wake up the worker thread
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    void RunInference(const EntityID entityID) {
        if (inferenceRunning.exchange(true)) {
            // Another inference is already running; skip
            return;
        }

        std::async(std::launch::async, [this, entityID]() {
            try {
                std::cout << "Starting inference for entity " << entityID << "\n";

                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);

                // Stable Diffusion logic
                sd_ctx_t *sd_context = InitializeStableDiffusionContext(entityID);
                if (!sd_context) {
                    throw std::runtime_error("Failed to initialize Stable Diffusion context!");
                }

                sd_image_t *image = GenerateImage(sd_context, entityID);
                if (!image) {
                    throw std::runtime_error("Failed to generate image!");
                }

                SaveImage(image->data, image->width, image->height, image->channel, entityID);

                free_sd_ctx(sd_context);
                std::cout << "Inference completed for entity " << entityID << "\n";

            } catch (const std::exception &e) {
                std::cerr << "Exception during inference: " << e.what() << std::endl;
            }

            // Mark inference as done and trigger the next
            inferenceRunning = false;
            TriggerNextInference();
        });
    }

    void TriggerNextInference() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (!inferenceQueue.empty()) {
            EntityID nextEntity = inferenceQueue.front();
            inferenceQueue.pop();
            RunInference(nextEntity);
        } else {
            StopWorker();
        }
    }

    void SaveImage(const unsigned char *data, int width, int height, int channels, EntityID entityID) {
        std::stringstream newPath;
        newPath << "./AniStudio_" << std::setw(5) << std::setfill('0') << saveCounter++ << ".png";
        if (!stbi_write_png(newPath.str().c_str(), width, height, channels, data, width * channels)) {
            std::cerr << "Failed to save image: " << newPath.str() << "\n";
        } else {
            std::cout << "Image saved successfully: " << newPath.str() << "\n";
        }
    }

    // Utility method to initialize Stable Diffusion text to image context
    sd_ctx_t *InitializeStableDiffusionContext(const EntityID entityID) {
        return new_sd_ctx(mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<CLipLComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<CLipGComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<T5XXLComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<DiffusionModelComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<VaeComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<TaesdComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<ControlnetComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<LoraComponent>(entityID).modelPath.c_str(),
                          mgr.GetComponent<EmbeddingComponent>(entityID).modelPath.c_str(), "",
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
                       mgr.GetComponent<CFGComponent>(entityID).cfg, mgr.GetComponent<CFGComponent>(entityID).guidance,
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
                       mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_end);
    }

    // Utility to validate component paths
    bool ArePathsValid(const EntityID entityID) {
        return std::filesystem::exists(mgr.GetComponent<ModelComponent>(entityID).modelPath) ||
               std::filesystem::exists(mgr.GetComponent<DiffusionModelComponent>(entityID).modelPath);
    }
};

} // namespace ECS
