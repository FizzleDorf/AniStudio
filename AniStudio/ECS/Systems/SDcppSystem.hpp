#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "ImageSystem.hpp"
#include "SDCPPComponents.h"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPool.hpp"
#include <png.h>
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

        SDCPPSystem(EntityManager& entityMgr, size_t numThreads = 0)
            : BaseSystem(entityMgr),
            threadPool(numThreads > 0 ? numThreads : std::thread::hardware_concurrency() / 2) {
            sysName = "SDCPPSystem";
            AddComponentSignature<LatentComponent>();
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
        // Thread pool for parallel processing
        Utils::ThreadPool threadPool;

        // Task counters
        std::atomic<size_t> activeInferenceTasks;
        std::atomic<size_t> activeConversionTasks;

        // Queues
        std::vector<QueueItem> inferenceQueue;
        std::vector<QueueItem> convertQueue;
        std::atomic<bool> pauseWorker{ false };

        // Synchronization
        std::mutex queueMutex;

        // Friend classes for tasks
        friend class InferenceTask;
        friend class ConvertTask;

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

        // Helper methods to create tasks
        std::shared_ptr<Utils::Task> CreateInferenceTask(EntityID entityID, const nlohmann::json& metadata);
        std::shared_ptr<Utils::Task> CreateConversionTask(EntityID entityID);

        // Core methods that will be used by task classes
        bool RunInference(EntityID entityID, const nlohmann::json& metadata);
        bool ConvertToGGUF(EntityID entityID);

        // SD context initialization
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

        // Save image 
        void SaveImage(const unsigned char* data, int width, int height, int channels, EntityID entityID, const nlohmann::json& metadata) {
            ImageComponent& imageComp = mgr.GetComponent<ImageComponent>(entityID);

            // Create a path for the filename and ensure .png extension
            std::filesystem::path filename(imageComp.fileName);
            if (filename.extension() != ".png") {
                filename.replace_extension(".png");
                imageComp.fileName = filename.string();
            }

            // Construct full path by combining directory with filename
            std::filesystem::path directoryPath = imageComp.filePath;
            std::filesystem::path fullPath = directoryPath / filename;

            try {
                // Ensure the directory exists, or create it
                if (!std::filesystem::exists(directoryPath)) {
                    std::filesystem::create_directories(directoryPath);
                    std::cout << "Directory created: " << directoryPath << '\n';
                }

                // Find the highest existing index in the directory
                int highestIndex = 0;
                for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                    if (entry.path().extension() == ".png") {
                        std::string filenameStr = entry.path().stem().string();
                        size_t lastDashPos = filenameStr.find_last_of('-');
                        if (lastDashPos != std::string::npos) {
                            try {
                                int index = std::stoi(filenameStr.substr(lastDashPos + 1));
                                if (index > highestIndex) {
                                    highestIndex = index;
                                }
                            }
                            catch (const std::invalid_argument&) {
                                // Skip files that do not have the expected format
                            }
                        }
                    }
                }

                // Increment the index for the new file
                highestIndex++;
                std::ostringstream formattedIndex;
                formattedIndex << std::setw(5) << std::setfill('0') << highestIndex; // Format with leading zeros

                std::string newFilename = filename.stem().string() + "-" + formattedIndex.str() + ".png";

                // Update the full path with the new filename
                fullPath = directoryPath / newFilename;

                // Write the image to file
                if (!stbi_write_png(fullPath.string().c_str(), width, height, channels, data, width * channels)) {
                    std::cerr << "Failed to save image: " << fullPath << std::endl;
                    return;
                }

                // Update component with full path including filename
                imageComp.filePath = fullPath.string();

                // Allow time for file to be fully written
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // Write metadata
                WriteMetadataToPNG(entityID, metadata);

                std::cout << "Image saved successfully: \"" << fullPath << "\"" << std::endl;

                // Add to loaded media
                mgr.GetSystem<ImageSystem>()->SaveImage(entityID, imageComp.filePath);

            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory: " << e.what() << '\n';
            }
        }

        // Write metadata to PNG
        bool WriteMetadataToPNG(EntityID entity, const nlohmann::json& metadata) {
            // Get the image component
            const auto& imageComp = mgr.GetComponent<ImageComponent>(entity);

            // Create a new JSON object that combines entity metadata with the additional fields
            nlohmann::json combinedMetadata = metadata; // Use the passed metadata (from EntityManager)

            // Add the additional metadata fields
            combinedMetadata["version"] = "1.0";
            combinedMetadata["software"] = "AniStudio";
            combinedMetadata["timestamp"] = std::time(nullptr);

            std::cout << "Writing metadata to: " << imageComp.filePath << std::endl;

            // Open the PNG file for reading
            FILE* fp = fopen(imageComp.filePath.c_str(), "rb");
            if (!fp) {
                std::cerr << "Failed to open PNG for reading: " << imageComp.filePath << std::endl;
                return false;
            }

            // Verify PNG signature
            unsigned char header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                std::cerr << "Not a valid PNG file" << std::endl;
                fclose(fp);
                return false;
            }
            fseek(fp, 0, SEEK_SET);

            // Initialize PNG read structures
            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                std::cerr << "Failed to create PNG read struct" << std::endl;
                fclose(fp);
                return false;
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                std::cerr << "Failed to create PNG info struct" << std::endl;
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(png))) {
                std::cerr << "Error during PNG read initialization" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            // Get image info
            png_uint_32 width, height;
            int bit_depth, color_type;
            png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

            // Create temporary file
            std::string tempFile = imageComp.filePath + ".tmp";
            FILE* out = fopen(tempFile.c_str(), "wb");
            if (!out) {
                std::cerr << "Failed to create temporary file" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            // Initialize PNG write structures
            png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!pngWrite) {
                std::cerr << "Failed to create PNG write struct" << std::endl;
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite) {
                std::cerr << "Failed to create PNG write info struct" << std::endl;
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite))) {
                std::cerr << "Error during PNG write initialization" << std::endl;
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(pngWrite, out);

            // Copy IHDR
            png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            // Set up metadata chunks
            std::string metadataStr = combinedMetadata.dump(); // Use the combined metadata
            std::vector<png_text> texts;

            // Parameters text chunk
            png_text paramText;
            paramText.compression = PNG_TEXT_COMPRESSION_NONE;
            // PNG_TEXT_COMPRESSION_zTXt; Use zlib compression for the metadata
            paramText.key = const_cast<char*>("parameters");
            paramText.text = const_cast<char*>(metadataStr.c_str());
            paramText.text_length = metadataStr.length();
            paramText.itxt_length = 0;
            paramText.lang = nullptr;
            paramText.lang_key = nullptr;
            texts.push_back(paramText);

            // Software identifier
            png_text softwareText;
            softwareText.compression = PNG_TEXT_COMPRESSION_NONE;
            softwareText.key = const_cast<char*>("Software");
            softwareText.text = const_cast<char*>("AniStudio");
            softwareText.text_length = 9;
            softwareText.itxt_length = 0;
            softwareText.lang = nullptr;
            softwareText.lang_key = nullptr;
            texts.push_back(softwareText);

            // Write the text chunks
            std::cout << "Writing " << texts.size() << " text chunks..." << std::endl;
            png_set_text(pngWrite, infoWrite, texts.data(), texts.size());

            // Write PNG header
            png_write_info(pngWrite, infoWrite);

            // Copy image data
            std::vector<png_byte> row(png_get_rowbytes(png, info));
            for (png_uint_32 y = 0; y < height; y++) {
                png_read_row(png, row.data(), nullptr);
                png_write_row(pngWrite, row.data());
            }

            // Finish writing
            png_write_end(pngWrite, infoWrite);

            // Clean up
            png_destroy_write_struct(&pngWrite, &infoWrite);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(out);
            fclose(fp);

            // Replace original with new file
            try {
                std::filesystem::path originalPath(imageComp.filePath);
                std::filesystem::path tempPath(tempFile);

                // Remove original file
                if (std::filesystem::exists(originalPath)) {
                    std::filesystem::remove(originalPath);
                }

                // Rename temp file to original
                std::filesystem::rename(tempPath, originalPath);
                std::cout << "Successfully wrote metadata to PNG" << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }
    };

    // Concrete Task implementation for inference
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

    // Concrete Task implementation for conversion
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

    // Implementation of task creation methods
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

            // Save the image
            SaveImage(image->data, image->width, image->height, image->channel, entityID, metadata);

            // Clean up
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
                mgr.DestroyEntity(entityID);
            }

            return false;
        }
    }

    // Convert to GGUF
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
