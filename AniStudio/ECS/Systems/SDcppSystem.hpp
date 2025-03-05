#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "ImageSystem.hpp"
#include "SDCPPComponents.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPool.hpp"
#include "PngMetadataUtils.hpp"  // Include the PNG metadata utility
#include <stb_image.h>
#include <stb_image_write.h>

namespace ECS {

    // Forward declarations
    class InferenceTask;
    class ConvertTask;

    class SDCPPSystem : public BaseSystem {
    public:
        struct QueueItem {
            EntityID entityID = 0;
            bool processing = false;
            nlohmann::json metadata = nlohmann::json();
            std::shared_ptr<Utils::Task> task;
        };

        // Constructor, destructor, and public methods remain unchanged
        SDCPPSystem(EntityManager& entityMgr, size_t numThreads = 0)
            : BaseSystem(entityMgr),
            threadPool(numThreads > 0 ? numThreads : std::thread::hardware_concurrency() / 2) {
            sysName = "SDCPPSystem";
            AddComponentSignature<LatentComponent>();
            AddComponentSignature<OutputImageComponent>();
            AddComponentSignature<InputImageComponent>();

            activeInferenceTasks = 0;
            activeConversionTasks = 0;
        }

        ~SDCPPSystem() {
            // Wait for all active tasks to complete
            std::unique_lock<std::mutex> lock(queueMutex);

            // Cancel all tasks
            for (auto& item : inferenceQueue) {
                if (item.task) {
                    item.task->cancel();
                }
            }

            lock.unlock();

            // Give tasks time to gracefully terminate
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Public methods (unchanged)
        void QueueInference(const EntityID entityID) {
            std::lock_guard<std::mutex> lock(queueMutex);

            // Create a queue item
            QueueItem item;
            item.entityID = entityID;
            item.processing = false;
            item.metadata = mgr.SerializeEntity(entityID);

            inferenceQueue.push_back(std::move(item));
            std::cout << "Entity " << entityID << " queued for inference." << std::endl;
        }

        void QueueConversion(const EntityID entityID) {
            std::lock_guard<std::mutex> lock(queueMutex);

            // Create a queue item for conversion
            QueueItem item;
            item.entityID = entityID;
            item.processing = false;

            convertQueue.push_back(std::move(item));
            std::cout << "Entity " << entityID << " queued for conversion." << std::endl;
        }

        void Update(const float deltaT) override {
            // Process queues - start any pending tasks
            ProcessQueues();

            // Update status of running tasks
            CheckTaskCompletion();
        }

        void RemoveFromQueue(const size_t index) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (index < inferenceQueue.size() && !inferenceQueue[index].processing) {
                inferenceQueue.erase(inferenceQueue.begin() + index);
            }
        }

        void MoveInQueue(const size_t fromIndex, const size_t toIndex) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (fromIndex >= inferenceQueue.size() || toIndex >= inferenceQueue.size())
                return;
            if (inferenceQueue[fromIndex].processing)
                return;

            QueueItem item = inferenceQueue[fromIndex];
            inferenceQueue.erase(inferenceQueue.begin() + fromIndex);
            inferenceQueue.insert(inferenceQueue.begin() + toIndex, item);
        }

        std::vector<QueueItem> GetQueueSnapshot() {
            std::lock_guard<std::mutex> lock(queueMutex);
            return inferenceQueue;
        }

        void StopCurrentTask() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& item : inferenceQueue) {
                if (item.processing && item.task) {
                    item.task->cancel();
                }
            }
        }

        void ClearQueue() {
            std::lock_guard<std::mutex> lock(queueMutex);
            // Remove all non-processing tasks
            inferenceQueue.erase(
                std::remove_if(inferenceQueue.begin(), inferenceQueue.end(),
                    [](const QueueItem& item) { return !item.processing; }),
                inferenceQueue.end()
            );
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
        size_t GetActiveInferenceCount() const { return activeInferenceTasks; }
        size_t GetActiveConversionCount() const { return activeConversionTasks; }

    private:
        // Private member variables
        Utils::ThreadPool threadPool;
        std::atomic<size_t> activeInferenceTasks;
        std::atomic<size_t> activeConversionTasks;
        std::vector<QueueItem> inferenceQueue;
        std::vector<QueueItem> convertQueue;
        std::atomic<bool> pauseWorker{ false };
        std::mutex queueMutex;

        // Friend classes for tasks
        friend class InferenceTask;
        friend class ConvertTask;

        // Queue processing methods
        void ProcessQueues() {
            std::lock_guard<std::mutex> lock(queueMutex);

            // If paused, don't process any new tasks
            if (pauseWorker) {
                return;
            }

            // Process conversion queue first (higher priority)
            for (auto& item : convertQueue) {
                if (!item.processing && activeConversionTasks < 1) {
                    // Create a conversion task
                    item.task = CreateConversionTask(item.entityID);
                    item.processing = true;
                    activeConversionTasks++;

                    // Add to thread pool
                    threadPool.addTask(item.task);
                }
            }

            // Process inference queue only if no active inference tasks
            if (activeInferenceTasks < 1) {
                for (auto& item : inferenceQueue) {
                    if (!item.processing) {
                        // Create an inference task
                        item.task = CreateInferenceTask(item.entityID, item.metadata);
                        item.processing = true;
                        activeInferenceTasks++;

                        // Add to thread pool
                        threadPool.addTask(item.task);

                        // Only start one task per update
                        break;
                    }
                }
            }
        }

        void CheckTaskCompletion() {
            std::unique_lock<std::mutex> lock(queueMutex);

            // Check inference queue
            for (auto it = inferenceQueue.begin(); it != inferenceQueue.end();) {
                if (it->processing && it->task && it->task->isDone()) {
                    activeInferenceTasks--;
                    it = inferenceQueue.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Check conversion queue
            for (auto it = convertQueue.begin(); it != convertQueue.end();) {
                if (it->processing && it->task && it->task->isDone()) {
                    activeConversionTasks--;
                    it = convertQueue.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        // Helper methods to create tasks (unchanged)
        std::shared_ptr<Utils::Task> CreateInferenceTask(EntityID entityID, const nlohmann::json& metadata);
        std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID);

        // Core methods that will be used by task classes
        bool RunInference(EntityID entityID, const nlohmann::json& metadata);
        bool ConvertToGGUF(EntityID entityID);

        // SD context initialization (unchanged)
        sd_ctx_t* InitializeStableDiffusionContext(EntityID entityID) {
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

        // Generate image (unchanged)
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

        // Save image - SIMPLIFIED using PngMetadata utility
        void SaveImage(const unsigned char* data, int width, int height, int channels, EntityID entity, const nlohmann::json& metadata) {
            // Make sure entity has OutputImageComponent
            if (!mgr.HasComponent<OutputImageComponent>(entity)) {
                // Add it if it doesn't exist yet
                mgr.AddComponent<OutputImageComponent>(entity);
            }

            OutputImageComponent& imageComp = mgr.GetComponent<OutputImageComponent>(entity);

            // Copy the raw data to the output component
            if (imageComp.imageData) {
                stbi_image_free(imageComp.imageData); // Free any existing data
                imageComp.imageData = nullptr;
            }

            // Allocate new memory for the image data
            size_t dataSize = width * height * channels;
            imageComp.imageData = (unsigned char*)malloc(dataSize);
            if (!imageComp.imageData) {
                std::cerr << "Failed to allocate memory for image data" << std::endl;
                return;
            }

            // Copy the data
            memcpy(imageComp.imageData, data, dataSize);

            // Update image component properties
            imageComp.width = width;
            imageComp.height = height;
            imageComp.channels = channels;

            // Create a path for the filename and ensure .png extension
            std::filesystem::path filename(imageComp.fileName);
            if (filename.extension() != ".png") {
                filename.replace_extension(".png");
                imageComp.fileName = filename.string();
            }

            try {
                // Use PngMetadata utility to get a unique filename
                std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
                    imageComp.fileName,
                    imageComp.filePath,
                    ".png"
                );

                // Update the component with the new path
                std::filesystem::path fullPath(uniqueFilePath);
                imageComp.fileName = fullPath.filename().string();
                imageComp.filePath = uniqueFilePath;

                // Save the image
                if (!stbi_write_png(uniqueFilePath.c_str(), width, height, channels, data, width * channels)) {
                    std::cerr << "Failed to save image: " << uniqueFilePath << std::endl;
                    return;
                }

                // Allow time for file to be fully written
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Create metadata with additional fields and write to PNG
                nlohmann::json enhancedMetadata = Utils::PngMetadata::CreateGenerationMetadata(metadata);
                Utils::PngMetadata::WriteMetadataToPNG(uniqueFilePath, enhancedMetadata);

                std::cout << "Image saved successfully: \"" << uniqueFilePath << "\"" << std::endl;

                static std::mutex imageMutex;
                {
                    std::lock_guard<std::mutex> lock(imageMutex);

                    // First create a new entity instead of modifying the current one
                    EntityID newImageEntity = mgr.AddNewEntity();

                    // Add ImageComponent to the new entity
                    auto& newImageComp = mgr.AddComponent<ImageComponent>(newImageEntity);

                    // Copy data from the output component to the new image component
                    newImageComp.filePath = imageComp.filePath;
                    newImageComp.fileName = imageComp.fileName;
                    newImageComp.width = imageComp.width;
                    newImageComp.height = imageComp.height;
                    newImageComp.channels = imageComp.channels;

                    // Allocate and copy image data
                    if (imageComp.imageData) {
                        newImageComp.imageData = (unsigned char*)malloc(dataSize);
                        if (newImageComp.imageData) {
                            memcpy(newImageComp.imageData, imageComp.imageData, dataSize);
                        }
                    }

                    // Get the ImageSystem
                    auto imageSystem = mgr.GetSystem<ImageSystem>();
                    if (imageSystem) {
                        // Load the image into the new entity
                        imageSystem->LoadImage(newImageComp);

                        // Clean up the original entity's output component 
                        // but keep the entity itself for potential reuse
                        mgr.RemoveComponent<OutputImageComponent>(entity);
                    }
                }
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
        ConvertTask(SDCPPSystem* system, EntityID entityID)
            : system(system), entityID(entityID) {
        }

        void execute() override {
            if (!system) return;

            // Run the conversion
            try {
                bool success = system->ConvertToGGUF(entityID);
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
    };

    // Implementation of task creation methods (unchanged)
    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateInferenceTask(EntityID entityID, const nlohmann::json& metadata) {
        return std::make_shared<InferenceTask>(this, entityID, metadata);
    }

    inline std::shared_ptr<Utils::Task> SDCPPSystem::CreateConversionTask(EntityID entityID) {
        return std::make_shared<ConvertTask>(this, entityID);
    }

    // Implementation of core methods

    // Run inference
    inline bool SDCPPSystem::RunInference(EntityID entityID, const nlohmann::json& metadata) {
        sd_ctx_t* sd_context = nullptr;
        try {
            std::cout << "Starting inference for Entity " << entityID << std::endl;

            if (!mgr.HasComponent<OutputImageComponent>(entityID)) {
                mgr.AddComponent<OutputImageComponent>(entityID);
                auto& outputComp = mgr.GetComponent<OutputImageComponent>(entityID);

                if (mgr.HasComponent<ImageComponent>(entityID)) {
                    const auto& imgComp = mgr.GetComponent<ImageComponent>(entityID);
                    outputComp.fileName = imgComp.fileName;
                    outputComp.filePath = imgComp.filePath;
                }
                else {
                    // Use defaults if no image component exists
                    outputComp.fileName = "AniStudio.png";
                    outputComp.filePath = filePaths.defaultProjectPath;
                }
            }

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

            SaveImage(image->data, image->width, image->height, image->channel, entityID, metadata);

            free_sd_ctx(sd_context);

            std::cout << "Inference completed for Entity " << entityID << std::endl;
            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "Exception during inference: " << e.what() << std::endl;

            if (sd_context) {
                free_sd_ctx(sd_context);
            }

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                // Only destroy the entity if it's not being used by another system
                if (mgr.HasComponent<OutputImageComponent>(entityID)) {
                    mgr.DestroyEntity(entityID);
                }
            }

            return false;
        }
    }

    // Convert to GGUF (unchanged)
    inline bool SDCPPSystem::ConvertToGGUF(EntityID entityID) {
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

} // namespace ECS