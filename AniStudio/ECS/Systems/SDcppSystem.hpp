#include "ECS.h"
#include "ImageSystem.hpp"
#include "pch.h"
#include "stable-diffusion.h"
#include <filesystem>
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
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
    SDCPPSystem() : inferenceRunning(false), stopWorker(false), workerThreadRunning(false) {
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<InputImageComponent>();
    }

    ~SDCPPSystem() { StopWorker(); }

    void QueueInference(EntityID entityID) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            inferenceQueue.push(entityID);
        }
        queueCondition.notify_one();
        std::cout << "Entity " << entityID << " queued for inference." << std::endl;

        // Ensure the worker is only started if not already running
        if (!workerThreadRunning.load()) {
            StartWorker();
        }
    }

    void Update(float deltaT) override {}

    void StopWorker() {
        {
            std::lock_guard<std::mutex> lock(workerMutex);
            stopWorker = true;
            queueCondition.notify_all();
        }

        if (workerThread.joinable()) {
            workerThread.join();
        }

        stopWorker = false;
        workerThreadRunning = false;
    }

private:
    std::queue<EntityID> inferenceQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> inferenceRunning;
    std::atomic<bool> stopWorker;
    std::atomic<bool> workerThreadRunning;
    std::mutex workerMutex;
    std::thread workerThread;
    int saveCounter = 0;

    void StartWorker() {
        workerThread = std::thread([this]() {
            try {
                while (true) {
                    EntityID entityID;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        // Wait for the queue to have work or the stop signal
                        queueCondition.wait(lock, [this]() { return stopWorker || !inferenceQueue.empty(); });

                        if (stopWorker && inferenceQueue.empty()) {
                            break;
                        }

                        // Process one entity at a time
                        entityID = inferenceQueue.front();
                        inferenceQueue.pop();
                    }

                    // Only process if no other inference is running
                    if (!inferenceRunning.exchange(true)) {
                        RunInference(entityID);
                        inferenceRunning = false; // Allow next inference after current finishes
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << "Worker thread exception: " << e.what() << std::endl;
            }
            workerThreadRunning = false; // Reset flag when the thread exits
        });
        workerThread.detach(); // Detach to avoid join issues
    }

    void RunInference(const EntityID entityID) {
        sd_ctx_t *sd_context = nullptr;
        try {
            std::cout << "Starting inference for Entity " << entityID << std::endl;

            sd_set_log_callback(LogCallback, nullptr);
            sd_set_progress_callback(ProgressCallback, nullptr);

            sd_context = InitializeStableDiffusionContext(entityID);
            if (!sd_context) {
                mgr.DestroyEntity(entityID);
                throw std::runtime_error("Failed to initialize Stable Diffusion context!");
            }

            sd_image_t *image = GenerateImage(sd_context, entityID);
            if (!image) {
                mgr.DestroyEntity(entityID);
                throw std::runtime_error("Failed to generate image!");
            }

            SaveImage(image->data, image->width, image->height, image->channel, entityID);

            free_sd_ctx(sd_context);
            std::cout << "Inference completed for Entity " << entityID << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Exception during inference: " << e.what() << std::endl;
            if (sd_context) {
                free_sd_ctx(sd_context); // Free context in case of error
            }
        }
    }


    void SaveImage(const unsigned char *data, int width, int height, int channels, EntityID entityID) {
        ImageComponent &imageComp = mgr.GetComponent<ImageComponent>(entityID);

        // Construct the full path by combining directory and filename
        std::filesystem::path directoryPath = imageComp.filePath;
        std::filesystem::path fullPath = directoryPath / imageComp.fileName;
        std::cout << "Directory Path: " << directoryPath << std::endl;
        std::cout << "File Name: " << imageComp.fileName << std::endl;
        std::cout << "Full Path: " << fullPath << std::endl;

        // Ensure the directory exists
        try {
            if (!std::filesystem::exists(directoryPath)) {
                std::filesystem::create_directories(directoryPath);
                std::cout << "Directory created: " << directoryPath << '\n';
            }
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Error creating directory: " << e.what() << '\n';
            return;
        }

        // Save the image using the full path
        if (!stbi_write_png(fullPath.string().c_str(), width, height, channels, data, width * channels)) {
            std::cerr << "Failed to save image: " << fullPath << std::endl;
        } else {
            std::cout << "Image saved successfully: " << fullPath << std::endl;
        }
    }



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
};

} // namespace ECS