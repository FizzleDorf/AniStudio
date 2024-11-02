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
        AddComponentSignature<ImageComponent>();
    }

    void Start() override {}

    void Inference(const EntityID &entityID) {
        if (inferenceRunning.load()) {
            std::cout << "Inference is already running; skipping this request." << std::endl;
            return;
        }
        std::cout << "Inference is started" << std::endl;
        inferenceRunning.store(true);

        // Launch asynchronous task with std::async
        inferenceFuture = std::async(std::launch::async, [this, entityID]() {
            try {
                std::cout << mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str() << std::endl;
                sd_set_log_callback(LogCallback, nullptr);
                sd_set_progress_callback(ProgressCallback, nullptr);
//std::string model_path = mgr.GetComponent<ModelComponent>(entityID).modelPath;
                std::string txxl_path = "";
                std::string clip_l_path = "";
                std::string clip_g_path = "";
                std::string diffusion_model_path = "";
                std::string vae_path = "";
                std::string control_net_path = "";
                std::string lora_model_dir = "";
                std::string embed_dir = "";
                std::string taesd_path = "";
                std::string stacked_id_embed_dir = "";
                bool vae_decode_only = false;
                bool vae_tiling = false;
                bool free_params_immediately = true;
                int n_threads = 4;
                sd_type_t wtype = sd_type_t::SD_TYPE_F16;
                rng_type_t rng_type = rng_type_t::STD_DEFAULT_RNG;
                schedule_t schedule = schedule_t::DEFAULT;
                bool keep_clip_on_cpu = true;
                bool keep_control_net_cpu = false;
                bool keep_vae_on_cpu = false;

                // Image Inference
                std::string pos_prompt = "1girl";
                std::string neg_prompt = "";
                int clip_skip = 0;
                float cfg_scale = 1.0f;
                float guidance_scale = 1.0f;
                int width = 512;
                int height = 768;
                sample_method_t sample_method = EULER;
                int sample_steps = 20;
                int seed = 31337;
                int batch_count = 1;
                // void *control_cond = nullptr;
                float control_strength = 0.0f;
                float style_strength = 0.0f;
                bool normalize_input = false;
                std::string input_id_images_path = "";

                // Initialize Stable Diffusion context
                sd_ctx_t *sd_context = new_sd_ctx(
                    mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(), clip_l_path.c_str(),
                    clip_g_path.c_str(), txxl_path.c_str(), "", vae_path.c_str(),
                    taesd_path.c_str(), control_net_path.c_str(),
                    lora_model_dir.c_str(), embed_dir.c_str(),
                    stacked_id_embed_dir.c_str(), vae_decode_only, vae_tiling,
                    free_params_immediately, n_threads, wtype,
                    rng_type, schedule, keep_clip_on_cpu,
                    keep_control_net_cpu, keep_vae_on_cpu);

                if (!sd_context) {
                    throw std::runtime_error(
                        "Failed to initialize Stable Diffusion context! Please check paths and parameters.");
                }
                std::cout << "Stable Diffusion context initialized successfully." << std::endl;

                // Perform image generation
                sd_image_t *image = txt2img(
                    sd_context, pos_prompt.c_str(), neg_prompt.c_str(),
                    clip_skip, cfg_scale, guidance_scale, width,
                    height, sample_method, sample_steps, seed,
                    batch_count, nullptr, control_strength, style_strength,
                    normalize_input, input_id_images_path.c_str());

                if (!image) {
                    throw std::runtime_error("Failed to generate image! Please verify input parameters.");
                }
                std::cout << "Image generated successfully." << std::endl;

                // Clean up resources
                free_sd_ctx(sd_context);
                
            } catch (const std::exception &e) {
                std::cerr << "Exception in async task: " << e.what() << std::endl;
            }
            mgr.DestroyEntity(entityID);
            inferenceRunning.store(false);
        });       
    }

    void Update() override {
        if (!inferenceRunning.load() && !inferenceQueue.empty()) {
            std::cout << "num entities: " << mgr.GetEntityCount() << std::endl;
            std::lock_guard<std::mutex> lock(queueMutex); // Lock while accessing the queue
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
