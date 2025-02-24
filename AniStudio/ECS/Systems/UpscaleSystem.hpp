#include "ECS.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "../Components/SDCPPComponents/SDCPPComponents.h"
#include <filesystem>
#include <future> // Include for std::future and std::async
#include <vector>

namespace ECS {
class UpscaleSystem : public BaseSystem {
public:
    UpscaleSystem(EntityManager &entityMgr) : BaseSystem(entityMgr), inferenceRunning(false) {
        sysName = "UpscaleSystem";
        AddComponentSignature<EsrganComponent>();
    }

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
                    inputImage,
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

    void Update(const float deltaT) override {
        std::lock_guard<std::mutex> lock(queueMutex); // Lock while modifying the queue
        if (!inferenceRunning.load() && !inferenceQueue.empty()) {
            EntityID entityID = inferenceQueue.front();
            inferenceQueue.pop();
            if (std::filesystem::exists(mgr.GetComponent<EsrganComponent>(entityID).modelPath)) {
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
    std::queue<EntityID> inferenceQueue; // Queue to hold entity IDs for inference
    std::mutex queueMutex;
    std::mutex inferenceMutex;
    std::mutex mgrMutex;
    std::atomic<bool> inferenceRunning{false}; // Atomic flag to track if inference is running
    std::future<void> inferenceFuture;         // Future for tracking the async inference task
};
} // namespace ECS
