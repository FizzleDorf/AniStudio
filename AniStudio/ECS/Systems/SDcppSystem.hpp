#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "ImageComponent.hpp"
#include "ImageUtils.hpp"
#include "SDCPPComponents.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPool.hpp"
#include "PngMetadataUtils.hpp"
#include <stb_image.h>
#include <stb_image_write.h>

namespace ECS {

    // Forward declarations
    class InferenceTask;
    class ConvertTask;

    // static rng variables
    // TODO: use as a util instead
    static STDDefaultRNG rng;
    static std::random_device rd;
    static bool initialized = false;

    class SDCPPSystem : public BaseSystem {
    public:
        enum class TaskType {
            Inference,
            Conversion,
            Img2Img,
            Upscaling
        };

        struct QueueItem {
            EntityID entityID = 0;
            bool processing = false;
            nlohmann::json metadata = nlohmann::json();
            std::shared_ptr<Utils::Task> task;
            TaskType taskType;
        };

        // Function to generate a random seed using STDDefaultRNG from sdcpp
        // TODO: use as a util instead
        uint64_t generateRandomSeed() {
            
            // Seed the RNG with a random device if not already seeded
            if (!initialized) {
                rng.manual_seed(rd());
                initialized = true;
            }

            // Get random numbers
            std::vector<float> random_values = rng.randn(1);

            // Convert to a positive integer seed
            uint64_t seed = static_cast<uint64_t>(std::abs(random_values[0] * UINT32_MAX)) % INT32_MAX;
            return seed > 0 ? seed : 1; // Ensure seed is positive
        }

        // Constructor, destructor, and public methods remain unchanged
        SDCPPSystem(EntityManager& entityMgr, size_t numThreads = 0)
            : BaseSystem(entityMgr),
            threadPool(numThreads > 0 ? numThreads : std::thread::hardware_concurrency() / 2) {
            sysName = "SDCPPSystem";
            AddComponentSignature<LatentComponent>();
            AddComponentSignature<OutputImageComponent>();
            AddComponentSignature<InputImageComponent>();

            activeTasks = 0;
        }

        ~SDCPPSystem() {
            // Wait for all active tasks to complete
            std::unique_lock<std::mutex> lock(queueMutex);

            // Cancel all tasks
            for (auto& item : taskQueue) {
                if (item.task) {
                    item.task->cancel();
                }
            }

            lock.unlock();

            // Give tasks time to gracefully terminate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Public methods
        void QueueTask(const EntityID entityID, const TaskType taskType) {
            // Validate entity exists first
            if (!mgr.GetEntitiesSignatures().count(entityID)) {
                std::cerr << "Error: Entity " << entityID << " does not exist!" << std::endl;
                return;
            }

            try {
                // try serializing entity before locking mutex
                QueueItem item;
                
                // Check if we need to generate a random seed
                if (taskType == TaskType::Inference || taskType == TaskType::Img2Img) {

                    // Access the sampler component
                    if (mgr.HasComponent<SamplerComponent>(entityID)) {
                        auto& samplerComp = mgr.GetComponent<SamplerComponent>(entityID);

                        // Generate random seed if needed
                        if (samplerComp.seed < 0) {
                            uint64_t newSeed = generateRandomSeed();
                            samplerComp.seed = static_cast<int>(newSeed);

                            std::cout << "Generated random seed: " << samplerComp.seed << std::endl;

                            // Update metadata with new seed for logging
                            if (item.metadata.contains("components") && item.metadata["components"].is_array()) {
                                for (auto& comp : item.metadata["components"]) {
                                    if (comp.contains("Sampler")) {
                                        comp["Sampler"]["seed"] = samplerComp.seed;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                item.metadata = mgr.SerializeEntity(entityID);
                std::cout << "Successfully serialized entity " << entityID << std::endl;

                // lock and add to queue
                std::lock_guard<std::mutex> lock(queueMutex);

                item.entityID = entityID;
                item.processing = false;
                item.taskType = taskType;

                // Print the queue size before adding
                std::cout << "Current queue size: " << taskQueue.size() << std::endl;

                // Add to queue with normal copying instead of move to see if that helps
                taskQueue.push_back(item);

                std::cout << "Entity " << entityID << " queued for inference. New queue size: " << taskQueue.size() << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in QueueTask: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Unknown exception in QueueTask!" << std::endl;
            }
        }

        void Update(const float deltaT) override {
            // Process queues - start any pending tasks
            ProcessQueues();

            // Update status of running tasks
            CheckTaskCompletion();
        }

        void RemoveFromQueue(const size_t index) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (index < taskQueue.size() && !taskQueue[index].processing) {
                mgr.DestroyEntity(taskQueue[index].entityID);
                taskQueue.erase(taskQueue.begin() + index);
            }
        }

        void MoveInQueue(const size_t fromIndex, const size_t toIndex) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size())
                return;
            if (taskQueue[fromIndex].processing)
                return;

            QueueItem item = taskQueue[fromIndex];
            taskQueue.erase(taskQueue.begin() + fromIndex);
            taskQueue.insert(taskQueue.begin() + toIndex, item);
        }

        std::vector<QueueItem> GetQueueSnapshot() {
            std::lock_guard<std::mutex> lock(queueMutex);
            return taskQueue;
        }

        void StopCurrentTask() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& item : taskQueue) {
                if (item.processing && item.task) {
                    item.task->cancel();
                }
            }
        }

        void ClearQueue() {
            for (size_t i = taskQueue.size() - 1; i > 0; --i) {
                RemoveFromQueue(i);
            }
        }

        void PauseWorker() {
            std::lock_guard<std::mutex> lock(queueMutex);
            pauseWorker = true;

            std::cout << "Worker paused. Tasks will continue running but no new tasks will be started." << std::endl;
        }

        void ResumeWorker() {
            std::lock_guard<std::mutex> lock(queueMutex);
            pauseWorker = false;

            std::cout << "Worker resumed. New tasks will now be processed." << std::endl;
        }

        // Thread pool stats
        size_t GetNumThreads() const { return threadPool.size(); }
        size_t GetQueuedTaskCount() const { return threadPool.getQueueSize(); }
        size_t GetActiveTaskCount() const { return activeTasks; }

    private:
        // Private member variables
        Utils::ThreadPool threadPool;
        std::atomic<size_t> activeTasks;
        std::vector<QueueItem> taskQueue;
        std::atomic<bool> pauseWorker{ false };
        std::mutex queueMutex;

        // Friend classes for tasks
        friend class InferenceTask;
        friend class ConvertTask;
        friend class Img2ImgTask;
        friend class UpscalingTask;

        // Queue processing methods
        // Update ProcessQueues method
        void ProcessQueues() {
            std::lock_guard<std::mutex> lock(queueMutex);

            // If paused, don't process any new tasks
            if (pauseWorker) {
                return;
            }

            // Process task queue
            for (auto& item : taskQueue) {
                if (!item.processing && activeTasks < 1) {

                    // Create a task based on the task type
                    switch (item.taskType) {
                    case TaskType::Inference:
                        item.task = CreateInferenceTask(item.entityID, item.metadata);
                        break;
                    case TaskType::Conversion:
                        item.task = CreateConversionTask(item.entityID, item.metadata);
                        break;
                    case TaskType::Img2Img:
                        item.task = CreateImg2ImgTask(item.entityID, item.metadata);
                        break;
                    case TaskType::Upscaling:
                        item.task = CreateUpscalingTask(item.entityID, item.metadata);
                        break;
                    default:
                        continue;
                    }

                    item.processing = true;
                    activeTasks++;

                    // Add to thread pool
                    threadPool.addTask(item.task);

                    // Only start one task per update
                    break;
                }
            }
        }

        void CheckTaskCompletion() {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Create a list of tasks to be removed
            std::vector<size_t> tasksToRemove;

            for (size_t i = 0; i < taskQueue.size(); i++) {
                auto& item = taskQueue[i];
                if (item.processing && item.task && item.task->isDone()) {
                    activeTasks--;
                    tasksToRemove.push_back(i);
                }
            }

            // Remove tasks in reverse order to avoid index shifting
            for (auto it = tasksToRemove.rbegin(); it != tasksToRemove.rend(); ++it) {
                taskQueue.erase(taskQueue.begin() + *it);
            }
        }

        // Helper methods to create tasks
        std::shared_ptr<Utils::Task> CreateInferenceTask(EntityID entityID, const nlohmann::json& metadata);
        std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID, const nlohmann::json& metadata);
        std::shared_ptr<Utils::Task> CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata);
        std::shared_ptr<Utils::Task> CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata);

        // Core methods that will be used by task classes
        bool RunInference(EntityID entityID, const nlohmann::json& metadata);
        bool ConvertToGGUF(EntityID entityID, const nlohmann::json& metadata);
        bool RunImg2Img(EntityID entityID, const nlohmann::json& metadata);
        bool RunUpscaling(EntityID entityID, const nlohmann::json& metadata);

        // SD context initialization
        sd_ctx_t* InitializeStableDiffusionContext(EntityID entityID) {
            return new_sd_ctx(mgr.GetComponent<ModelComponent>(entityID).modelPath.c_str(),
                mgr.GetComponent<ClipLComponent>(entityID).modelPath.c_str(),
                mgr.GetComponent<ClipGComponent>(entityID).modelPath.c_str(),
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

        // Generate image
        sd_image_t* GenerateImage(sd_ctx_t* context, EntityID entityID) {
            return txt2img(
                context,
                mgr.GetComponent<PromptComponent>(entityID).posPrompt.c_str(),
                mgr.GetComponent<PromptComponent>(entityID).negPrompt.c_str(),
                mgr.GetComponent<ClipSkipComponent>(entityID).clipSkip,
                mgr.GetComponent<SamplerComponent>(entityID).cfg,
                mgr.GetComponent<GuidanceComponent>(entityID).guidance,
                mgr.GetComponent<GuidanceComponent>(entityID).eta,
                mgr.GetComponent<LatentComponent>(entityID).latentWidth,
                mgr.GetComponent<LatentComponent>(entityID).latentHeight,
                mgr.GetComponent<SamplerComponent>(entityID).current_sample_method,
                mgr.GetComponent<SamplerComponent>(entityID).steps,
                mgr.GetComponent<SamplerComponent>(entityID).seed,
                mgr.GetComponent<LatentComponent>(entityID).batchSize,
                nullptr,
                0.0f,
                0.0f,
                false,
                "",
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers_count,
                mgr.GetComponent<LayerSkipComponent>(entityID).slg_scale,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_start,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_end
            );
        }

        void SaveImage(const unsigned char* data, int width, int height, int channels, EntityID entity, const nlohmann::json& metadata) {
            try {
                // Get the component for storing image data
                auto& imageComp = mgr.GetComponent<OutputImageComponent>(entity);

                // Update image dimensions from the actual generated data
                imageComp.width = width;
                imageComp.height = height;
                imageComp.channels = channels;

                // Set up default image properties if not already set
                if (imageComp.fileName.empty()) {
                    imageComp.fileName = "AniStudio.png";
                }

                if (imageComp.filePath.empty()) {
                    imageComp.filePath = filePaths.defaultProjectPath;
                }

                // Ensure PNG extension
                std::filesystem::path fileNamePath(imageComp.fileName);
                if (fileNamePath.extension() != ".png") {
                    fileNamePath.replace_extension(".png");
                    imageComp.fileName = fileNamePath.string();
                }

                // Use PngMetadata utility to get a unique filename
                std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
                    imageComp.fileName,
                    imageComp.filePath,
                    ".png"
                );

                // Update the component with the path info
                std::filesystem::path fullPath(uniqueFilePath);
                imageComp.fileName = fullPath.filename().string();
                imageComp.filePath = uniqueFilePath;

                // First, store a copy of the raw image data in the component
                if (imageComp.imageData) {
                    free(imageComp.imageData);
                    imageComp.imageData = nullptr;
                }

                // Copy the data to the component
                size_t dataSize = width * height * channels;
                imageComp.imageData = static_cast<unsigned char*>(malloc(dataSize));
                if (imageComp.imageData) {
                    memcpy(imageComp.imageData, data, dataSize);
                }
                else {
                    std::cerr << "Failed to allocate memory for image data" << std::endl;
                }

                // Save the image to disk
                bool success = Utils::ImageUtils::SaveImage(
                    uniqueFilePath,
                    width,
                    height,
                    channels,
                    data
                );

                if (!success) {
                    std::cerr << "Failed to save image: " << uniqueFilePath << std::endl;
                    return;
                }

                // Allow time for file to be fully written
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Create metadata with additional fields and write to PNG
                nlohmann::json enhancedMetadata = Utils::PngMetadata::CreateGenerationMetadata(metadata);
                Utils::PngMetadata::WriteMetadataToPNG(uniqueFilePath, enhancedMetadata);

                std::cout << "Image saved successfully: \"" << uniqueFilePath << "\"" << std::endl;

                // Texture will be generated during next update by ImageSystem since we have imageData set
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory: " << e.what() << '\n';
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in SaveImage: " << e.what() << '\n';
            }
        }

    };

    // Task classes
    class InferenceTask : public Utils::Task {
    public:
        InferenceTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
            : system(system), entityID(entityID), metadata(metadata) {
        }

        void execute() override {
            if (!system) return;

            // Run the inference
            try {
                // Mark the task as done when it completes (even if there's an error)
                bool success = system->RunInference(entityID, metadata);
                markDone();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in inference task: " << e.what() << std::endl;
                markDone();
            }
        }

    private:
        SDCPPSystem* system;
        EntityID entityID;
        nlohmann::json metadata;
    };

    class ConvertTask : public Utils::Task {
    public:
        ConvertTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
            : system(system), entityID(entityID), metadata(metadata) {
        }

        void execute() override {
            if (!system) return;

            // Run the conversion
            try {
                bool success = system->ConvertToGGUF(entityID, metadata);
                markDone();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in conversion task: " << e.what() << std::endl;
                markDone();
            }
        }

    private:
        SDCPPSystem* system;
        EntityID entityID;
        nlohmann::json metadata;
    };

    class Img2ImgTask : public Utils::Task {
    public:
        Img2ImgTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
            : system(system), entityID(entityID), metadata(metadata) {
        }

        void execute() override {
            if (!system) return;

            try {
                bool success = system->RunImg2Img(entityID, metadata);
                markDone();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in img2img task: " << e.what() << std::endl;
                markDone();
            }
        }

    private:
        SDCPPSystem* system;
        EntityID entityID;
        nlohmann::json metadata;
    };

    class UpscalingTask : public Utils::Task {
    public:
        UpscalingTask(SDCPPSystem* system, EntityID entityID, const nlohmann::json& metadata)
            : system(system), entityID(entityID), metadata(metadata) {
        }

        void execute() override {
            if (!system) return;

            try {
                bool success = system->RunUpscaling(entityID, metadata);
                markDone();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in upscaling task: " << e.what() << std::endl;
                markDone();
            }
        }

    private:
        SDCPPSystem* system;
        EntityID entityID;
        nlohmann::json metadata;
    };

    // Implementation of task creation methods
    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateInferenceTask(EntityID entityID, const nlohmann::json& metadata) {
        return std::make_shared<InferenceTask>(this, entityID, metadata);
    }

    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateConversionTask(EntityID entityID, const nlohmann::json& metadata) {
        return std::make_shared<ConvertTask>(this, entityID, metadata);
    }

    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateImg2ImgTask(EntityID entityID, const nlohmann::json& metadata) {
        return std::make_shared<Img2ImgTask>(this, entityID, metadata);
    }

    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateUpscalingTask(EntityID entityID, const nlohmann::json& metadata) {
        return std::make_shared<UpscalingTask>(this, entityID, metadata);
    }

    // Implementation of core methods

    // Run inference
    inline bool SDCPPSystem::RunInference(EntityID entityID, const nlohmann::json& metadata) {
        sd_ctx_t* sd_context = nullptr;
        try {
            std::cout << "Starting inference for Entity " << entityID << std::endl;

            // Initialize Stable Diffusion context
            sd_context = InitializeStableDiffusionContext(entityID);
            if (!sd_context) {
                throw std::runtime_error("Failed to initialize Stable Diffusion context!");
            }

            // Generate image
            sd_image_t* image = GenerateImage(sd_context, entityID);
            if (!image) {
                throw std::runtime_error("Failed to generate image!");
            }

            // Save the generated image - this now creates a new entity with ImageComponent
            SaveImage(image->data, image->width, image->height, image->channel, entityID, metadata);

            // Cleanup SD context
            free_sd_ctx(sd_context);

            std::cout << "Inference completed for Entity " << entityID << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during inference: " << e.what() << std::endl;

            if (sd_context) {
                free_sd_ctx(sd_context);
            }

            return false;
        }
    }

    // Convert to GGUF
    inline bool SDCPPSystem::ConvertToGGUF(EntityID entityID, const nlohmann::json& metadata) {
        try {
            std::cout << "Starting conversion for Entity " << entityID << std::endl;

            // Get model paths and settings
            std::string inputPath = mgr.GetComponent<ModelComponent>(entityID).modelPath;
            std::string vaePath = mgr.GetComponent<VaeComponent>(entityID).modelPath;
            sd_type_t type = mgr.GetComponent<SamplerComponent>(entityID).current_type_method;

            // Validate input path
            if (inputPath.empty()) {
                throw std::runtime_error("Input model path is empty");
            }

            // Create output path with type suffix
            std::filesystem::path inPath(inputPath);
            std::string outPath = inPath.parent_path().string() + "/" +
                inPath.stem().string() + "_" +
                std::string(type_method_items[type]) + ".gguf";

            // Perform conversion
            bool result;
            if (vaePath.empty()) {
                // Convert without VAE
                result = convert(inputPath.c_str(), nullptr, outPath.c_str(), type);
            }
            else {
                // Convert with VAE
                result = convert(inputPath.c_str(), vaePath.c_str(), outPath.c_str(), type);
            }

            if (!result) {
                throw std::runtime_error("Failed to convert Model: " + inputPath);
            }

            std::cout << "Successfully converted model to: " << outPath << std::endl;

            // Clean up
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                mgr.DestroyEntity(entityID);
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during conversion: " << e.what() << std::endl;

            // Clean up
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                mgr.DestroyEntity(entityID);
            }

            return false;
        }
    }

    inline bool ECS::SDCPPSystem::RunImg2Img(EntityID entityID, const nlohmann::json& metadata) {
        sd_ctx_t* sd_context = nullptr;
        try {
            std::cout << "Starting img2img inference for Entity " << entityID << std::endl;

            // Initialize Stable Diffusion context
            sd_context = InitializeStableDiffusionContext(entityID);
            if (!sd_context) {
                throw std::runtime_error("Failed to initialize Stable Diffusion context!");
            }

            // Check if we have input image data
            if (!mgr.HasComponent<InputImageComponent>(entityID) ||
                !mgr.GetComponent<InputImageComponent>(entityID).imageData) {
                throw std::runtime_error("Input image required for img2img generation!");
            }

            // Get components
            auto& inputComp = mgr.GetComponent<InputImageComponent>(entityID);

            // Prepare input image
            sd_image_t init_image = {
                static_cast<uint32_t>(inputComp.width),
                static_cast<uint32_t>(inputComp.height),
                static_cast<uint32_t>(inputComp.channels),
                inputComp.imageData
            };

            // Prepare mask image if we have MaskImageComponent
            sd_image_t mask_image = { 0 };
            float denoiseStrength = 0.75f; // Default strength

            if (mgr.HasComponent<MaskImageComponent>(entityID)) {
                auto& maskComp = mgr.GetComponent<MaskImageComponent>(entityID);
                if (maskComp.imageData) {
                    mask_image.width = static_cast<uint32_t>(maskComp.width);
                    mask_image.height = static_cast<uint32_t>(maskComp.height);
                    mask_image.channel = static_cast<uint32_t>(maskComp.channels);
                    mask_image.data = maskComp.imageData;
                }
                // Use the mask component's value for denoise strength
                denoiseStrength = maskComp.value;
            }

            // Generate image
            sd_image_t* image = img2img(
                sd_context,
                init_image,
                mask_image,
                mgr.GetComponent<PromptComponent>(entityID).posPrompt.c_str(),
                mgr.GetComponent<PromptComponent>(entityID).negPrompt.c_str(),
                mgr.GetComponent<ClipSkipComponent>(entityID).clipSkip,
                mgr.GetComponent<SamplerComponent>(entityID).cfg,
                mgr.GetComponent<GuidanceComponent>(entityID).guidance,
                mgr.GetComponent<GuidanceComponent>(entityID).eta,
                mgr.GetComponent<LatentComponent>(entityID).latentWidth,
                mgr.GetComponent<LatentComponent>(entityID).latentHeight,
                mgr.GetComponent<SamplerComponent>(entityID).current_sample_method,
                mgr.GetComponent<SamplerComponent>(entityID).steps,
                denoiseStrength,
                mgr.GetComponent<SamplerComponent>(entityID).seed,
                mgr.GetComponent<LatentComponent>(entityID).batchSize,
                nullptr,  // control_cond
                0.0f,     // control_strength
                0.0f,     // style_strength
                false,    // normalize_input
                "",       // input_id_images_path
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layers_count,
                mgr.GetComponent<LayerSkipComponent>(entityID).slg_scale,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_start,
                mgr.GetComponent<LayerSkipComponent>(entityID).skip_layer_end
            );

            if (!image) {
                throw std::runtime_error("Failed to generate image!");
            }

            // Save the generated image
            SaveImage(image->data, image->width, image->height, image->channel, entityID, metadata);

            // Cleanup SD context
            free_sd_ctx(sd_context);

            std::cout << "Img2Img inference completed for Entity " << entityID << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during img2img inference: " << e.what() << std::endl;

            if (sd_context) {
                free_sd_ctx(sd_context);
            }

            return false;
        }
    }

    inline bool ECS::SDCPPSystem::RunUpscaling(EntityID entityID, const nlohmann::json& metadata) {
        upscaler_ctx_t* upscaler_context = nullptr;
        try {
            std::cout << "Starting upscaling for Entity " << entityID << std::endl;

            // Check if we have input image data
            if (!mgr.HasComponent<InputImageComponent>(entityID) ||
                !mgr.GetComponent<InputImageComponent>(entityID).imageData) {
                throw std::runtime_error("Input image required for upscaling!");
            }

            // Get ESRGAN path and settings
            auto& esrganComp = mgr.GetComponent<EsrganComponent>(entityID);
            if (esrganComp.modelPath.empty()) {
                throw std::runtime_error("ESRGAN model path is empty!");
            }

            // Initialize upscaler context - use default thread count from system
            upscaler_context = new_upscaler_ctx(
                esrganComp.modelPath.c_str(),
                mgr.GetComponent<SamplerComponent>(entityID).n_threads
            );

            if (!upscaler_context) {
                throw std::runtime_error("Failed to initialize upscaler context!");
            }

            // Get input image component
            auto& inputComp = mgr.GetComponent<InputImageComponent>(entityID);

            // Create input image
            sd_image_t input_image = {
                static_cast<uint32_t>(inputComp.width),
                static_cast<uint32_t>(inputComp.height),
                static_cast<uint32_t>(inputComp.channels),
                inputComp.imageData
            };

            // Perform upscaling
            sd_image_t upscaled_image = upscale(
                upscaler_context,
                input_image,
                esrganComp.upscaleFactor
            );

            // Save the upscaled image
            SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
                upscaled_image.channel, entityID, metadata);

            // Cleanup upscaler context
            free_upscaler_ctx(upscaler_context);

            std::cout << "Upscaling completed for Entity " << entityID << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception during upscaling: " << e.what() << std::endl;

            if (upscaler_context) {
                free_upscaler_ctx(upscaler_context);
            }

            return false;
        }
    }
} // namespace ECS