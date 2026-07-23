#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "SDCPPComponents.h"
#include "Components.h"
#include "SDCPPUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPoolSystem.hpp"
#include "SettingsSystem.hpp"
#include "FilePathSystem.hpp"
#include "ModelCacheSystem.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <optional>
#include <future>
#include <thread>
#include <mutex>
#include <random>

namespace ECS {

    class SDCPPSystem : public BaseSystem {
    public:
        enum class TaskType {
            Inference,
            Conversion,
            Img2Img,
            Img2Vid,
            Edit,
            Upscaling
        };

        struct QueueItem {
            EntityID entityID = 0;
            bool processing = false;
            TaskType taskType;
            QueueItem() = default;
            QueueItem(const QueueItem&) = default;
            QueueItem& operator=(const QueueItem&) = default;
        };

        struct TaskData {
            EntityID entityID = 0;
            bool processing = false;
            bool cancelled = false;
            TaskType taskType;
            nlohmann::json metadata;
            std::string fullPath;
            std::future<bool> result;
            sd_ctx_t* sdContext = nullptr;

            TaskData() = default;
            TaskData(TaskData&& other) noexcept
                : entityID(other.entityID), processing(other.processing), cancelled(other.cancelled),
                taskType(other.taskType), metadata(std::move(other.metadata)),
                fullPath(std::move(other.fullPath)), result(std::move(other.result)),
                sdContext(other.sdContext) {
                other.sdContext = nullptr;
            }
            TaskData& operator=(TaskData&& other) noexcept {
                if (this != &other) {
                    entityID = other.entityID;
                    processing = other.processing;
                    cancelled = other.cancelled;
                    taskType = other.taskType;
                    metadata = std::move(other.metadata);
                    fullPath = std::move(other.fullPath);
                    result = std::move(other.result);
                    sdContext = other.sdContext;
                    other.sdContext = nullptr;
                }
                return *this;
            }
            TaskData(const TaskData&) = delete;
            TaskData& operator=(const TaskData&) = delete;
            ~TaskData() {
                sdContext = nullptr;
            }
            void Cancel() {
                cancelled = true;
                if (sdContext) sd_cancel_generation(sdContext, SD_CANCEL_ALL);
            }
        };

        SDCPPSystem(EntityManager& entityMgr)
            : BaseSystem(entityMgr), pauseWorker(false), hasActiveTask(false), clearRequested(false) {
            sysName = "SDCPPSystem";
            m_filePathSystem = mgr.GetSystem<FilePathSystem>();
        }

        ~SDCPPSystem() { Shutdown(); }

        void Shutdown() {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                shuttingDown = true;
                pauseWorker = true;
            }
            StopCurrentTask();
            ClearAllTasks();
            if (workerThread.joinable()) workerThread.join();
            if (m_threadPool) m_threadPool->terminateAll();
        }

        void TerminateImmediately() {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                shuttingDown = true;
                pauseWorker = true;
            }
            ClearAllTasks();
            if (m_threadPool) m_threadPool->terminateAll();
        }

        void QueueTask(EntityID entityID, TaskType taskType) {
            if (!mgr.IsEntityValid(entityID)) {
                std::cerr << "[QueueTask] Invalid entity\n";
                return;
            }

            auto settingsSys = mgr.GetSystem<SettingsSystem>();
            if (settingsSys) {
                EntityID settingsEntity = settingsSys->GetSettingsEntity();
                if (mgr.IsEntityValid(settingsEntity) && mgr.HasComponent<SDCPPSettingsComponent>(settingsEntity)) {
                    auto& globalSettings = mgr.GetComponent<SDCPPSettingsComponent>(settingsEntity);
                    if (!mgr.HasComponent<SDCPPSettingsComponent>(entityID)) {
                        mgr.AddComponent<SDCPPSettingsComponent>(entityID);
                    }
                    auto& taskSettings = mgr.GetComponent<SDCPPSettingsComponent>(entityID);
                    taskSettings.lora_apply_mode = globalSettings.lora_apply_mode;
                    taskSettings.enable_mmap = globalSettings.enable_mmap;
                    taskSettings.flash_attn = globalSettings.flash_attn;
                    taskSettings.diffusion_flash_attn = globalSettings.diffusion_flash_attn;
                    taskSettings.tae_preview_only = globalSettings.tae_preview_only;
                    taskSettings.diffusion_conv_direct = globalSettings.diffusion_conv_direct;
                    taskSettings.vae_conv_direct = globalSettings.vae_conv_direct;
                    taskSettings.force_sdxl_vae_conv_scale = globalSettings.force_sdxl_vae_conv_scale;
                    taskSettings.vae_format = globalSettings.vae_format;
                    taskSettings.max_vram = globalSettings.max_vram;
                    taskSettings.stream_layers = globalSettings.stream_layers;
                    taskSettings.eager_load = globalSettings.eager_load;
                    taskSettings.backend = globalSettings.backend;
                    taskSettings.params_backend = globalSettings.params_backend;
                    taskSettings.split_mode = globalSettings.split_mode;
                    taskSettings.auto_fit = globalSettings.auto_fit;
                    taskSettings.rpc_servers = globalSettings.rpc_servers;
                }
            }

            std::lock_guard<std::mutex> lock(queueMutex);
            if (shuttingDown) return;
            TaskData taskData;
            taskData.entityID = entityID;
            taskData.processing = false;
            taskData.cancelled = false;
            taskData.taskType = taskType;

            if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
                taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
                if (mgr.HasComponent<SamplerComponent>(entityID)) {
                    auto& sampler = mgr.GetComponent<SamplerComponent>(entityID);
                    if (sampler.seed < 0) {
                        sampler.seed = static_cast<int>(std::random_device{}());
                    }
                }
            }
            try {
                taskData.metadata = mgr.SerializeEntity(entityID);
            }
            catch (...) {
                std::cerr << "Serialization failed\n";
                return;
            }
            taskQueue.push_back(std::move(taskData));
        }

        void Update(float deltaT) override {
            if (shuttingDown) return;
            if (clearRequested) {
                HandleClearRequest();
                clearRequested = false;
            }
            ProcessQueues();
            CheckTaskCompletion();
        }

        void RemoveFromQueue(size_t index) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (index < taskQueue.size() && !taskQueue[index].processing) {
                taskQueue.erase(taskQueue.begin() + index);
            }
        }

        void MoveInQueue(size_t fromIndex, size_t toIndex) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (fromIndex >= taskQueue.size() || toIndex >= taskQueue.size()) return;
            if (taskQueue[fromIndex].processing) return;
            TaskData task = std::move(taskQueue[fromIndex]);
            taskQueue.erase(taskQueue.begin() + fromIndex);
            taskQueue.insert(taskQueue.begin() + toIndex, std::move(task));
        }

        std::vector<QueueItem> GetQueueSnapshot() {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::vector<QueueItem> result;
            result.reserve(taskQueue.size());
            for (const auto& task : taskQueue) {
                QueueItem item;
                item.entityID = task.entityID;
                item.processing = task.processing;
                item.taskType = task.taskType;
                result.push_back(item);
            }
            return result;
        }

        void StopCurrentTask() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& task : taskQueue) {
                if (task.processing) {
                    task.Cancel();
                    break;
                }
            }
            pauseWorker = true;
        }

        void CancelCurrentTask() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& task : taskQueue) {
                if (task.processing) {
                    task.Cancel();
                    break;
                }
            }
        }

        void ClearQueuedTasks() {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.erase(std::remove_if(taskQueue.begin(), taskQueue.end(),
                [](const TaskData& t) { return !t.processing; }), taskQueue.end());
            if (taskQueue.empty()) {
                hasActiveTask = false;
            }
        }

        void ClearAllTasks() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& task : taskQueue) {
                if (task.processing) {
                    task.Cancel();
                }
            }
            taskQueue.clear();
            hasActiveTask = false;
            clearRequested = false;
        }

        void PauseWorker() { std::lock_guard<std::mutex> lock(queueMutex); pauseWorker = true; }
        void ResumeWorker() { std::lock_guard<std::mutex> lock(queueMutex); pauseWorker = false; }

        bool IsPaused() const { std::lock_guard<std::mutex> lock(queueMutex); return pauseWorker; }

        size_t GetNumThreads() const { return m_threadPool ? m_threadPool->getDiffusionPool().size() : 0; }
        size_t GetQueuedTaskCount() const { return m_threadPool ? m_threadPool->getDiffusionPool().queueSize() : 0; }
        size_t GetActiveTaskCount() const { return m_threadPool ? m_threadPool->getDiffusionPool().activeCount() : 0; }
        bool HasActiveTask() const { std::lock_guard<std::mutex> lock(queueMutex); return hasActiveTask; }
        size_t GetQueueSize() const { std::lock_guard<std::mutex> lock(queueMutex); return taskQueue.size(); }

        void Start() override {
            m_cacheSystem = mgr.GetSystem<ModelCacheSystem>();
            m_threadPool = mgr.GetSystem<ThreadPoolSystem>();
            if (!m_threadPool) {
                std::cerr << "[SDCPPSystem] ThreadPoolSystem not available\n";
            }
            workerThread = std::thread([this]() { WorkerThread(); });
        }

        void Destroy() override { Shutdown(); BaseSystem::Destroy(); }

    private:
        std::vector<TaskData> taskQueue;
        std::atomic<bool> pauseWorker{ false };
        std::atomic<bool> shuttingDown{ false };
        std::atomic<bool> clearRequested{ false };
        std::vector<EntityID> entitiesNeedingCleanup;
        mutable std::mutex queueMutex;
        std::thread workerThread;
        bool hasActiveTask{ false };
        std::thread::id activeThreadId{};
        std::shared_ptr<ThreadPoolSystem> m_threadPool;
        std::shared_ptr<FilePathSystem> m_filePathSystem;
        std::shared_ptr<ModelCacheSystem> m_cacheSystem;

        upscaler_ctx_t* CreateUpscalerContext(const nlohmann::json& metadata) {
            std::string esrganPath;
            bool direct = false;
            int n_threads = 4, tile_size = 64;
            std::string backend, params_backend;
            if (metadata.contains("components") && metadata["components"].is_array()) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains("Esrgan")) {
                        auto esrgan = comp["Esrgan"];
                        if (esrgan.contains("modelPath") && !esrgan["modelPath"].is_null())
                            esrganPath = esrgan["modelPath"].get<std::string>();
                        if (esrgan.contains("direct")) direct = esrgan["direct"].get<bool>();
                        if (esrgan.contains("n_threads")) n_threads = esrgan["n_threads"].get<int>();
                        if (esrgan.contains("tile_size")) tile_size = esrgan["tile_size"].get<int>();
                        if (esrgan.contains("backend") && !esrgan["backend"].is_null())
                            backend = esrgan["backend"].get<std::string>();
                        if (esrgan.contains("params_backend") && !esrgan["params_backend"].is_null())
                            params_backend = esrgan["params_backend"].get<std::string>();
                    }
                    if (comp.contains("Sampler")) {
                        auto sampler = comp["Sampler"];
                        if (sampler.contains("n_threads")) n_threads = sampler["n_threads"].get<int>();
                    }
                }
            }
            if (esrganPath.empty()) return nullptr;
            return new_upscaler_ctx(esrganPath.c_str(), direct, n_threads, tile_size,
                backend.empty() ? nullptr : backend.c_str(),
                params_backend.empty() ? nullptr : params_backend.c_str());
        }

        void LoadImageViaImageSystem(EntityID targetEntity, const std::string& filePath) {
            if (auto imgSys = mgr.GetSystem<ImageSystem>()) {
                if (!mgr.HasComponent<ImageComponent>(targetEntity))
                    mgr.AddComponent<ImageComponent>(targetEntity);
                imgSys->SetImage(targetEntity, filePath);
            }
        }

        void LoadVideoViaVideoSystem(const std::string& filePath) {
            if (auto vidSys = mgr.GetSystem<VideoSystem>()) {
                EntityID videoEntity = mgr.AddNewEntity();
                mgr.AddComponent<OutputVideoComponent>(videoEntity);
                auto& vidComp = mgr.GetComponent<OutputVideoComponent>(videoEntity);
                vidComp.filePath = filePath;
                vidComp.fileName = std::filesystem::path(filePath).filename().string();
                vidSys->SetVideo(videoEntity, filePath);
            }
        }

        void HandleClearRequest() {
            std::lock_guard<std::mutex> lock(queueMutex);
            ClearQueuedTasks();
        }

        static uint64_t GenerateSeed() {
            static std::random_device rd;
            static std::mt19937_64 gen(rd());
            return gen();
        }

        static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            SDCPP::ResourceManager res;
            sd_img_gen_params_t params;
            sd_img_gen_params_init(&params);
            if (!SDCPP::parseImageGenParams(metadata, params, res)) return false;
            if (params.seed < 0) params.seed = static_cast<int64_t>(GenerateSeed());
            sd_image_t* images = nullptr;
            int count = 0;
            bool ok = generate_image(context, &params, &images, &count);
            if (ok && count > 0 && images && images[0].data) {
                Utils::ImageUtils::SaveImage(fullPath, images[0].width, images[0].height, images[0].channel, images[0].data);
                Utils::PngMetadata::WriteMetadataToPNG(fullPath, metadata);
                free_sd_images(images, count);
                return true;
            }
            if (images) free_sd_images(images, count);
            return false;
        }

        static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            SDCPP::ResourceManager res;
            sd_img_gen_params_t params;
            sd_img_gen_params_init(&params);
            if (!SDCPP::parseImageGenParams(metadata, params, res)) return false;
            if (params.seed < 0) params.seed = static_cast<int64_t>(GenerateSeed());
            sd_image_t* images = nullptr;
            int count = 0;
            bool ok = generate_image(context, &params, &images, &count);
            if (ok && count > 0 && images && images[0].data) {
                Utils::ImageUtils::SaveImage(fullPath, images[0].width, images[0].height, images[0].channel, images[0].data);
                Utils::PngMetadata::WriteMetadataToPNG(fullPath, metadata);
                free_sd_images(images, count);
                return true;
            }
            if (images) free_sd_images(images, count);
            return false;
        }

        static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            SDCPP::ResourceManager res;
            sd_vid_gen_params_t params;
            sd_vid_gen_params_init(&params);
            if (!SDCPP::parseVideoGenParams(metadata, params, res)) return false;
            if (params.seed < 0) params.seed = static_cast<int64_t>(GenerateSeed());
            sd_image_t* frames = nullptr;
            int frameCount = 0;
            sd_audio_t* audio = nullptr;
            bool ok = generate_video(context, &params, &frames, &frameCount, &audio);
            if (ok && frameCount > 0 && frames) {
                for (int i = 0; i < frameCount; ++i) {
                    std::string framePath = fullPath + "_frame_" + std::to_string(i) + ".png";
                    Utils::ImageUtils::SaveImage(framePath, frames[i].width, frames[i].height, frames[i].channel, frames[i].data);
                }
                if (frameCount > 0) {
                    std::string firstFrame = fullPath + "_frame_0.png";
                    Utils::PngMetadata::WriteMetadataToPNG(firstFrame, metadata);
                }
                if (audio) free_sd_audio(audio);
                free(frames);
                return true;
            }
            if (frames) free(frames);
            if (audio) free_sd_audio(audio);
            return false;
        }

        static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            return RunImg2Img(metadata, fullPath, context);
        }

        static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath, upscaler_ctx_t* upscaler) {
            SDCPP::ResourceManager res;
            sd_ctx_params_t ctxParams;
            sd_ctx_params_init(&ctxParams);
            if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return false;
            std::string esrganPath = ctxParams.control_net_path ? ctxParams.control_net_path : "";
            if (esrganPath.empty()) return false;
            int n_threads = ctxParams.n_threads > 0 ? ctxParams.n_threads : 4;
            int tile_size = 64;
            bool direct = false;
            const char* backend = ctxParams.backend;
            const char* paramsBackend = ctxParams.params_backend;
            upscaler_ctx_t* ctx = new_upscaler_ctx(esrganPath.c_str(), direct, n_threads, tile_size, backend, paramsBackend);
            if (!ctx) return false;
            sd_image_t input = { 0,0,0,nullptr };
            if (metadata.contains("components") && metadata["components"].is_array()) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains("InputImage") && comp["InputImage"].contains("filePath")) {
                        std::string path = comp["InputImage"]["filePath"].get<std::string>();
                        int w, h, c;
                        unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
                        if (data) { input.width = w; input.height = h; input.channel = c; input.data = data; }
                        break;
                    }
                }
            }
            if (!input.data) { free_upscaler_ctx(ctx); return false; }
            uint32_t factor = 2;
            if (metadata.contains("components")) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains("Esrgan") && comp["Esrgan"].contains("upscaleFactor")) {
                        factor = comp["Esrgan"]["upscaleFactor"].get<uint32_t>();
                        break;
                    }
                }
            }
            sd_image_t* out = nullptr;
            int count = 0;
            bool ok = upscale(ctx, input, factor, &out, &count);
            if (ok && count > 0 && out && out[0].data) {
                Utils::ImageUtils::SaveImage(fullPath, out[0].width, out[0].height, out[0].channel, out[0].data);
                Utils::PngMetadata::WriteMetadataToPNG(fullPath, metadata);
                free_sd_images(out, count);
                stbi_image_free(input.data);
                free_upscaler_ctx(ctx);
                return true;
            }
            if (out) free_sd_images(out, count);
            stbi_image_free(input.data);
            free_upscaler_ctx(ctx);
            return false;
        }

        static bool RunConversion(const nlohmann::json& metadata) {
            SDCPP::ResourceManager res;
            sd_ctx_params_t ctxParams;
            sd_ctx_params_init(&ctxParams);
            if (!SDCPP::parseContextParams(metadata, ctxParams, res)) return false;
            std::string input = ctxParams.model_path ? ctxParams.model_path : "";
            std::string vae = ctxParams.vae_path ? ctxParams.vae_path : "";
            std::string output = input;
            if (!output.empty()) {
                output = std::filesystem::path(output).stem().string() + "_converted.gguf";
            }
            enum sd_type_t type = ctxParams.wtype;
            const char* rules = ctxParams.tensor_type_rules;
            bool convertName = true;
            if (metadata.contains("components")) {
                for (const auto& comp : metadata["components"]) {
                    if (comp.contains("Conversion") && comp["Conversion"].contains("convertName")) {
                        convertName = comp["Conversion"]["convertName"].get<bool>();
                    }
                }
            }
            return convert(input.c_str(), vae.c_str(), output.c_str(), type, rules, convertName);
        }

        bool IsVideoTask(TaskType taskType) const {
            return taskType == TaskType::Img2Vid || taskType == TaskType::Edit;
        }

        std::string GetOutputExtension(TaskType taskType) const {
            return (taskType == TaskType::Img2Vid || taskType == TaskType::Edit) ? ".mp4" : ".png";
        }

        void ProcessQueues() {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (pauseWorker || shuttingDown) return;
            if (taskQueue.empty() || hasActiveTask) return;
            if (!m_threadPool || !m_cacheSystem) return;

            auto& diffusionPool = m_threadPool->getDiffusionPool();

            for (auto it = taskQueue.begin(); it != taskQueue.end(); ++it) {
                auto& task = *it;
                if (task.processing) continue;

                if (!task.sdContext) {
                    if (task.taskType == TaskType::Inference || task.taskType == TaskType::Img2Img ||
                        task.taskType == TaskType::Img2Vid || task.taskType == TaskType::Edit) {
                        task.sdContext = m_cacheSystem->getOrCreateContext(task.metadata);
                        if (!task.sdContext) break;
                    }
                }

                if (task.fullPath.empty() && mgr.HasComponent<OutputImageComponent>(task.entityID)) {
                    auto& output = mgr.GetComponent<OutputImageComponent>(task.entityID);
                    std::string baseName = output.fileName;
                    std::string extension = GetOutputExtension(task.taskType);
                    size_t lastDot = baseName.find_last_of('.');
                    if (lastDot != std::string::npos) baseName = baseName.substr(0, lastDot);
                    std::string fullFileName = baseName + extension;
                    std::string outputDir = output.filePath;
                    if (!outputDir.empty() && std::filesystem::path(outputDir).has_extension())
                        outputDir = std::filesystem::path(outputDir).parent_path().string();
                    if (outputDir.empty()) {
                        if (m_filePathSystem) {
                            outputDir = m_filePathSystem->GetPath("DefaultProject");
                        }
                        if (outputDir.empty()) {
                            outputDir = std::filesystem::current_path().string();
                        }
                    }
                    task.fullPath = Utils::PngMetadata::CreateUniqueFilename(fullFileName, outputDir);
                }

                try {
                    switch (task.taskType) {
                    case TaskType::Inference: {
                        nlohmann::json metadata = task.metadata;
                        std::string fullPath = task.fullPath;
                        sd_ctx_t* sdContext = task.sdContext;
                        task.result = diffusionPool.submit(
                            [metadata, fullPath, sdContext]() -> bool {
                                return RunInference(metadata, fullPath, sdContext);
                            }
                        );
                        break;
                    }
                    case TaskType::Conversion: {
                        nlohmann::json metadata = task.metadata;
                        task.result = diffusionPool.submit(
                            [metadata]() -> bool {
                                return RunConversion(metadata);
                            }
                        );
                        break;
                    }
                    case TaskType::Img2Img: {
                        nlohmann::json metadata = task.metadata;
                        std::string fullPath = task.fullPath;
                        sd_ctx_t* sdContext = task.sdContext;
                        task.result = diffusionPool.submit(
                            [metadata, fullPath, sdContext]() -> bool {
                                return RunImg2Img(metadata, fullPath, sdContext);
                            }
                        );
                        break;
                    }
                    case TaskType::Img2Vid: {
                        nlohmann::json metadata = task.metadata;
                        std::string fullPath = task.fullPath;
                        sd_ctx_t* sdContext = task.sdContext;
                        task.result = diffusionPool.submit(
                            [metadata, fullPath, sdContext]() -> bool {
                                return RunImg2Vid(metadata, fullPath, sdContext);
                            }
                        );
                        break;
                    }
                    case TaskType::Edit: {
                        nlohmann::json metadata = task.metadata;
                        std::string fullPath = task.fullPath;
                        sd_ctx_t* sdContext = task.sdContext;
                        task.result = diffusionPool.submit(
                            [metadata, fullPath, sdContext]() -> bool {
                                return RunEdit(metadata, fullPath, sdContext);
                            }
                        );
                        break;
                    }
                    case TaskType::Upscaling: {
                        nlohmann::json metadata = task.metadata;
                        std::string fullPath = task.fullPath;
                        upscaler_ctx_t* sdContext = reinterpret_cast<upscaler_ctx_t*>(task.sdContext);
                        task.result = diffusionPool.submit(
                            [metadata, fullPath, sdContext]() -> bool {
                                return RunUpscaling(metadata, fullPath, sdContext);
                            }
                        );
                        break;
                    }
                    default:
                        continue;
                    }
                    task.processing = true;
                    hasActiveTask = true;
                    activeThreadId = std::this_thread::get_id();
                    break;
                }
                catch (...) {
                    it = taskQueue.erase(it);
                    break;
                }
            }
        }

        void CheckTaskCompletion() {
            if (taskQueue.empty()) return;
            std::vector<std::tuple<std::string, TaskType, EntityID>> completedTasks;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                for (auto it = taskQueue.begin(); it != taskQueue.end();) {
                    if (it->processing) {
                        if (it->cancelled) {
                            if (it->result.valid()) {
                                auto status = it->result.wait_for(std::chrono::milliseconds(0));
                                if (status == std::future_status::ready) {
                                    try { it->result.get(); }
                                    catch (...) {}
                                    it = taskQueue.erase(it);
                                    continue;
                                }
                            }
                            else {
                                it = taskQueue.erase(it);
                                continue;
                            }
                            ++it;
                            continue;
                        }
                        if (it->result.valid()) {
                            auto status = it->result.wait_for(std::chrono::milliseconds(0));
                            if (status == std::future_status::ready) {
                                EntityID entityID = it->entityID;
                                std::string fullPath = it->fullPath;
                                TaskType taskType = it->taskType;
                                bool success = false;
                                try {
                                    success = it->result.get();
                                }
                                catch (...) {}
                                if (!shuttingDown && success && std::filesystem::exists(fullPath)) {
                                    completedTasks.emplace_back(fullPath, taskType, entityID);
                                }
                                else {
                                    if (std::filesystem::exists(fullPath))
                                        std::filesystem::remove(fullPath);
                                }
                                it = taskQueue.erase(it);
                                hasActiveTask = false;
                            }
                            else {
                                ++it;
                            }
                        }
                        else {
                            ++it;
                        }
                    }
                    else {
                        ++it;
                    }
                }
                if (taskQueue.empty() && !hasActiveTask) {
                    activeThreadId = std::thread::id{};
                }
            }
            if (!shuttingDown) {
                for (const auto& [path, type, id] : completedTasks) {
                    ProcessCompletedTask(path, type, id);
                }
            }
        }

        void ProcessCompletedTask(const std::string& fullPath, TaskType taskType, EntityID entityID) {
            try {
                if (shuttingDown || !std::filesystem::exists(fullPath)) return;
                if (IsVideoTask(taskType)) {
                    LoadVideoViaVideoSystem(fullPath);
                }
                else {
                    EntityID newImageEntity = mgr.AddNewEntity();
                    mgr.AddComponent<ImageComponent>(newImageEntity);
                    LoadImageViaImageSystem(newImageEntity, fullPath);
                }
            }
            catch (...) {
                if (std::filesystem::exists(fullPath))
                    std::filesystem::remove(fullPath);
            }
        }

        void WorkerThread() {
            while (!shuttingDown) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    };

} // namespace ECS