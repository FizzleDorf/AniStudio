#pragma once

#include "Constants.hpp"
#include "ECS.h"
#include "ImageSystem.hpp"
#include "SDCPPComponents.h"
#include "pch.h"
#include "stable-diffusion.h"
#include <png.h>
#include <stb_image.h>
#include <stb_image_write.h>

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
    struct QueueItem {
        EntityID entityID = 0;
        bool processing = false;
        nlohmann::json metadata = nlohmann::json();
    };

    struct ConvertQueueItem {
        EntityID entityID = 0;
        bool processing = false;
    };

    SDCPPSystem(EntityManager &entityMgr)
        : BaseSystem(entityMgr), stopWorker(false), workerThreadRunning(false), taskRunning(false) {
        sysName = "SDCPPSystem";
        AddComponentSignature<LatentComponent>();
        AddComponentSignature<InputImageComponent>();
        StartWorker();
    }

    ~SDCPPSystem() { StopWorker(); }

    void QueueInference(const EntityID entityID) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            inferenceQueue.push_back({entityID, false, SerializeEntityComponents(entityID)});
        }
        std::cout << "metadata: " << '\n' << inferenceQueue.back().metadata << std::endl;
        queueCondition.notify_one();
        std::cout << "Entity " << entityID << " queued for inference." << std::endl;

        if (!workerThreadRunning.load()) {
            StartWorker();
        }
    }

    void QueueConversion(const EntityID entityID) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            convertQueue.push_back({entityID, false});
        }
        queueCondition.notify_one();
        std::cout << "Entity " << entityID << " queued for conversion." << std::endl;

        if (!workerThreadRunning.load()) {
            StartWorker();
        }
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

        auto item = inferenceQueue[fromIndex];
        inferenceQueue.erase(inferenceQueue.begin() + fromIndex);
        inferenceQueue.insert(inferenceQueue.begin() + toIndex, item);
    }

    std::vector<QueueItem> GetQueueSnapshot() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return inferenceQueue;
    }

    void Update(const float deltaT) override {}

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

    void StartWorker() {
        std::lock_guard<std::mutex> lock(workerMutex);
        if (workerThreadRunning) {
            return;
        }

        workerThreadRunning = true;
        workerThread = std::thread([this]() { WorkerLoop(); });
    }

private:
    std::vector<QueueItem> inferenceQueue;
    std::vector<ConvertQueueItem> convertQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> stopWorker;
    std::atomic<bool> workerThreadRunning;
    std::atomic<bool> taskRunning;
    std::mutex workerMutex;
    std::thread workerThread;

    void WorkerLoop() {
        while (workerThreadRunning) {

            std::cout << "QueueItem temp" << std::endl;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(
                    lock, [this]() { return stopWorker || !inferenceQueue.empty() || !convertQueue.empty(); });

                if (stopWorker) {
                    break;
                }

                // Prioritize conversion queue
                if (!convertQueue.empty()) 
                    convertQueue.front().processing = true;
                
                if (!inferenceQueue.empty())
                    inferenceQueue.front().processing = true;

            }

            try {
                if (!convertQueue.empty()) { // Check if we have a convert task
                    ConvertToGGUF(convertQueue.front());
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        mgr.DestroyEntity(convertQueue.front().entityID);
                        convertQueue.erase(convertQueue.begin());
                    }
                }

                if (!inferenceQueue.empty() && convertQueue.empty()) { // Check if we have an inference task
                    RunInference(inferenceQueue.front());
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        inferenceQueue.erase(inferenceQueue.begin());
                    }
                }

            } catch (const std::exception &e) {
                std::cerr << "Worker error: " << e.what() << std::endl;
            }
        }

        workerThreadRunning.store(false);
    }

    void RunInference(const QueueItem item) {
        if (taskRunning)
            return;

        taskRunning.store(true);

        sd_ctx_t *sd_context = nullptr;
        try {
            std::cout << "Starting inference for Entity " << item.entityID << std::endl;

            sd_set_log_callback(LogCallback, nullptr);
            sd_set_progress_callback(ProgressCallback, nullptr);

            sd_context = InitializeStableDiffusionContext(item.entityID);
            if (!sd_context)
                throw std::runtime_error("Failed to initialize Stable Diffusion context!");

            sd_image_t *image = GenerateImage(sd_context, item.entityID);
            if (!image)
                throw std::runtime_error("Failed to generate image!");

            SaveImage(image->data, image->width, image->height, image->channel, item);
            free_sd_ctx(sd_context);
            std::cout << "Inference completed for Entity " << item.entityID << std::endl;

        } catch (const std::exception &e) {
            std::cerr << "Exception during inference: " << e.what() << std::endl;
            if (sd_context) {
                free_sd_ctx(sd_context);
            }
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                mgr.DestroyEntity(item.entityID);
            }
        }
        taskRunning.store(false);
    }

    void ConvertToGGUF(const ConvertQueueItem item) {
        if (taskRunning)
            return;

        taskRunning.store(true);
        std::string inputPath = mgr.GetComponent<ModelComponent>(item.entityID).modelPath;
        std::string vaePath = mgr.GetComponent<VaeComponent>(item.entityID).modelPath;
        sd_type_t type = mgr.GetComponent<SamplerComponent>(item.entityID).current_type_method;

        // Validate input path
        if (inputPath.empty()) {
            std::cerr << "Input model path is empty" << std::endl;
            taskRunning.store(false);
            return;
        }

        // Create output path with type suffix
        std::filesystem::path inPath(inputPath);
        std::string outPath = inPath.parent_path().string() + "/" + inPath.stem().string() + "_" +
                              std::string(type_method_items[type]) + ".gguf";

        try {
            sd_set_log_callback(LogCallback, nullptr);
            bool result;

            if (vaePath.empty()) {
                // Convert without VAE
                result = convert(inputPath.c_str(), nullptr, outPath.c_str(), type);
            } else {
                // Convert with VAE
                result = convert(inputPath.c_str(), vaePath.c_str(), outPath.c_str(), type);
            }

            if (!result) {
                std::cerr << "Failed to convert Model: " << inputPath << std::endl;
                return;
            }

            std::cout << "Successfully converted model to: " << outPath << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Exception during conversion: " << e.what() << std::endl;
        }

        taskRunning.store(false);
    }

    void SaveImage(const unsigned char *data, int width, int height, int channels, const QueueItem item) {
        ImageComponent &imageComp = mgr.GetComponent<ImageComponent>(item.entityID);

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
            for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
                if (entry.path().extension() == ".png") {
                    std::string filenameStr = entry.path().stem().string();
                    size_t lastDashPos = filenameStr.find_last_of('-');
                    if (lastDashPos != std::string::npos) {
                        try {
                            int index = std::stoi(filenameStr.substr(lastDashPos + 1));
                            if (index > highestIndex) {
                                highestIndex = index;
                            }
                        } catch (const std::invalid_argument &) {
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
                mgr.DestroyEntity(item.entityID);
                return;
            }

            // Update component with full path including filename
            imageComp.filePath = fullPath.string();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!WriteMetadataToPNG(item.entityID, item.metadata)) {
                std::cerr << "Failed to write metadata to: " << fullPath << std::endl;
            } else {
                std::cout << "Successfully wrote metadata to: " << fullPath << std::endl;
            }

            std::cout << "Image saved successfully: \"" << fullPath << "\"" << std::endl;
            loadedMedia.AddImage(imageComp);

        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Error creating directory: " << e.what() << '\n';
            return;
        }
    }

    sd_ctx_t *InitializeStableDiffusionContext(EntityID entityID) {
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

    sd_image_t *GenerateImage(sd_ctx_t *context, EntityID entityID) {
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

    nlohmann::json SerializeEntityComponents(EntityID entity) {
        nlohmann::json componentData;

        // Helper lambda to check and serialize a component if it exists
        auto serializeComponent = [this](EntityID entity, auto componentType) {
            using T = decltype(componentType);
            if (mgr.HasComponent<T>(entity)) {
                const auto &comp = mgr.GetComponent<T>(entity);
                return comp.Serialize();
            }
            return nlohmann::json{};
        };

        // Serialize each component type
        componentData.merge_patch(serializeComponent(entity, ModelComponent{}));
        componentData.merge_patch(serializeComponent(entity, CLipLComponent{}));
        componentData.merge_patch(serializeComponent(entity, CLipGComponent{}));
        componentData.merge_patch(serializeComponent(entity, T5XXLComponent{}));
        componentData.merge_patch(serializeComponent(entity, DiffusionModelComponent{}));
        componentData.merge_patch(serializeComponent(entity, VaeComponent{}));
        componentData.merge_patch(serializeComponent(entity, TaesdComponent{}));
        componentData.merge_patch(serializeComponent(entity, ControlnetComponent{}));
        componentData.merge_patch(serializeComponent(entity, LoraComponent{}));
        componentData.merge_patch(serializeComponent(entity, LatentComponent{}));
        componentData.merge_patch(serializeComponent(entity, SamplerComponent{}));
        componentData.merge_patch(serializeComponent(entity, CFGComponent{}));
        componentData.merge_patch(serializeComponent(entity, PromptComponent{}));
        componentData.merge_patch(serializeComponent(entity, EmbeddingComponent{}));
        componentData.merge_patch(serializeComponent(entity, LayerSkipComponent{}));
        componentData.merge_patch(serializeComponent(entity, ImageComponent{}));

        return componentData;
    }

    bool WriteMetadataToPNG(EntityID entity, const nlohmann::json &metadata) {
        const auto &imageComp = mgr.GetComponent<ImageComponent>(entity);

        FILE *fp = fopen(imageComp.filePath.c_str(), "rb");
        if (!fp) {
            std::cerr << "Failed to open PNG for reading: " << imageComp.filePath << std::endl;
            return false;
        }

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            fclose(fp);
            return false;
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            fclose(fp);
            return false;
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        // Create temporary file
        std::string tempFile = imageComp.filePath + ".tmp";
        FILE *out = fopen(tempFile.c_str(), "wb");
        if (!out) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return false;
        }

        png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        png_infop infoWrite = png_create_info_struct(pngWrite);
        png_init_io(pngWrite, out);

        // Copy original PNG header
        png_uint_32 width, height;
        int bit_depth, color_type;
        png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);
        png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        // Set up metadata text
        std::string metadataStr = metadata.dump();
        png_text texts[2];

        // Main parameters chunk
        texts[0].compression = PNG_TEXT_COMPRESSION_NONE;
        texts[0].key = const_cast<char *>("parameters");
        texts[0].text = const_cast<char *>(metadataStr.c_str());
        texts[0].text_length = metadataStr.length();
        texts[0].itxt_length = 0;
        texts[0].lang = nullptr;
        texts[0].lang_key = nullptr;

        // Software identifier
        texts[1].compression = PNG_TEXT_COMPRESSION_NONE;
        texts[1].key = const_cast<char *>("Software");
        texts[1].text = const_cast<char *>("AniStudio");
        texts[1].text_length = 9;
        texts[1].itxt_length = 0;
        texts[1].lang = nullptr;
        texts[1].lang_key = nullptr;

        png_set_text(pngWrite, infoWrite, texts, 2);
        png_write_info(pngWrite, infoWrite);

        // Copy image data
        png_bytep row = new png_byte[png_get_rowbytes(png, info)];
        for (png_uint_32 y = 0; y < height; y++) {
            png_read_row(png, row, nullptr);
            png_write_row(pngWrite, row);
        }
        delete[] row;

        png_write_end(pngWrite, infoWrite);

        // Clean up
        png_destroy_write_struct(&pngWrite, &infoWrite);
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(out);
        fclose(fp);

        // Replace original with new file using proper filesystem calls
        std::filesystem::path originalPath(imageComp.filePath);
        std::filesystem::path tempPath(tempFile);
        std::filesystem::remove(originalPath);
        std::filesystem::rename(tempPath, originalPath);

        return true;
    }
};

} // namespace ECS
