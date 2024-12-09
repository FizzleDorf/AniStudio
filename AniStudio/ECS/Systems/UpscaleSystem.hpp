#include "../../../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <filesystem>
#include <future> // Include for std::future and std::async
#include <vector>

//static void LogCallback(sd_log_level_t level, const char *text, void *data) {
//    switch (level) {
//    case SD_LOG_DEBUG:
//        std::cout << "[DEBUG]: " << text;
//        break;
//    case SD_LOG_INFO:
//        std::cout << "[INFO]: " << text;
//        break;
//    case SD_LOG_WARN:
//        std::cout << "[WARNING]: " << text;
//        break;
//    case SD_LOG_ERROR:
//        std::cerr << "[ERROR]: " << text;
//        break;
//    default:
//        std::cerr << "[UNKNOWN LOG LEVEL]: " << text;
//        break;
//    }
//}
//
//static void ProgressCallback(int step, int steps, float time, void *data) {
//    std::cout << "Progress: Step " << step << " of " << steps << " | Time: " << time << "s" << std::endl;
//}

namespace ECS {
class UpscaleSystem : public BaseSystem {
public:
    UpscaleSystem() {
        AddComponentSignature<EsrganComponent>();
    }

    void Start() override {}

    void Inference(const EntityID entityID) {
        std::lock_guard<std::mutex> lock(inferenceMutex);
        if (inferenceRunning.load()) {
            std::cout << "Inference is already running; skipping this request."
                      << "\n";
            return;
        }
        std::cout << "Upscale is started"
                  << "\n";
        inferenceRunning.store(true);

        // Launch asynchronous task with std::async
        inferenceFuture = std::async(std::launch::async, [this, entityID]() {
            try {
                std::lock_guard<std::mutex> lock(mgrMutex);
                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);

                // Using heap allocation for context and image
                upscaler_ctx_t *upscale_context =
                    new_upscaler_ctx(
                    mgr.GetComponent<EsrganComponent>(entityID).modelPath.c_str(),
                    mgr.GetComponent<SamplerComponent>(entityID).n_threads
                    );

                if (!upscale_context) {
                    throw std::runtime_error(
                        "Failed to initialize upscale context! Please check paths and parameters.");
                }
                std::cout << "Upscale context initialized successfully."
                          << "\n";

                sd_image_t inputImage;
                inputImage.width = mgr.GetComponent<InputImageComponent>(entityID).width;
                inputImage.height = mgr.GetComponent<InputImageComponent>(entityID).height;
                inputImage.channel = mgr.GetComponent<InputImageComponent>(entityID).channels;
                inputImage.data = mgr.GetComponent<InputImageComponent>(entityID).imageData;
                 
                // Perform image generation
                sd_image_t image = upscale(
                    upscale_context, 
                    inputImage, // needs a sd_image
                    4
                );

                std::cout << "Image upscaled successfully."
                          << "\n";

                // Copy image data to a buffer to ensure it's available for saving
                int dataSize = image.width * image.height * image.channel;
                std::vector<unsigned char> imageData(image.data, image.data + dataSize);

                // Store the image properties
                mgr.GetComponent<ImageComponent>(entityID).imageData = image.data;
                mgr.GetComponent<ImageComponent>(entityID).width = image.width;
                mgr.GetComponent<ImageComponent>(entityID).height = image.height;
                mgr.GetComponent<ImageComponent>(entityID).channels = image.channel;

                // Debug output for verification
                std::cout << "Saving image with width: " << image.width << ", height: " << image.height
                          << ", channels: " << image.channel << "\n";

                // Save image using stb_image
                int stride = image.width * image.channel;
                stbi_write_png("./Anistudio.png", image.width, image.height, image.channel, imageData.data(),
                               stride);

                // Clean up resources
                free_upscaler_ctx(upscale_context);

            } catch (const std::exception &e) {
                std::cerr << "Exception in async task: " << e.what() << "\n";
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
        std::cout << "Entity Queued for inference."
                  << "\n";
    }

private:
    EntityManager &mgr = ECS::EntityManager::Ref();
    std::queue<EntityID> inferenceQueue; // Queue to hold entity IDs for inference
    std::mutex queueMutex;
    std::mutex inferenceMutex;
    std::mutex mgrMutex;
    std::atomic<bool> inferenceRunning{false}; // Atomic flag to track if inference is running
    std::future<void> inferenceFuture;         // Future for tracking the async inference task
};
} // namespace ECS
