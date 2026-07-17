#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "SDCPPComponents.h"
#include "Components.h"
#include "Txt2Img.hpp"
#include "Img2Img.hpp"
#include "Img2Vid.hpp"
#include "Edit.hpp"
#include "Upscaling.hpp"
#include "Conversion.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "ContextUtils.hpp"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPoolSystem.hpp"
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
            QueueItem(const QueueItem& other) = default;
            QueueItem& operator=(const QueueItem& other) = default;
        };

        struct TaskData {
            EntityID entityID = 0;
            bool processing = false;
            TaskType taskType;
            nlohmann::json metadata;
            std::string fullPath;
            std::future<bool> result;

            union {
                sd_ctx_t* sdContext;
                upscaler_ctx_t* upscalerContext;
            };

            bool contextAcquired = false;
            bool modelLoading = false;

            enum ContextType {
                NoContext,
                SDContext,
                UpscalerContext
            } contextType = NoContext;

            TaskData() : sdContext(nullptr), contextType(NoContext) {}

            TaskData(TaskData&& other) noexcept
                : entityID(other.entityID), processing(other.processing), taskType(other.taskType),
                metadata(std::move(other.metadata)), fullPath(std::move(other.fullPath)),
                result(std::move(other.result)), contextAcquired(other.contextAcquired),
                modelLoading(other.modelLoading), contextType(other.contextType) {
                switch (contextType) {
                case SDContext: sdContext = other.sdContext; other.sdContext = nullptr; break;
                case UpscalerContext: upscalerContext = other.upscalerContext; other.upscalerContext = nullptr; break;
                default: sdContext = nullptr; break;
                }
            }

            TaskData& operator=(TaskData&& other) noexcept {
                if (this != &other) {
                    CleanupContext();
                    entityID = other.entityID;
                    processing = other.processing;
                    taskType = other.taskType;
                    metadata = std::move(other.metadata);
                    fullPath = std::move(other.fullPath);
                    result = std::move(other.result);
                    contextAcquired = other.contextAcquired;
                    modelLoading = other.modelLoading;
                    contextType = other.contextType;
                    switch (contextType) {
                    case SDContext: sdContext = other.sdContext; other.sdContext = nullptr; break;
                    case UpscalerContext: upscalerContext = other.upscalerContext; other.upscalerContext = nullptr; break;
                    default: sdContext = nullptr; break;
                    }
                }
                return *this;
            }

            TaskData(const TaskData&) = delete;
            TaskData& operator=(const TaskData&) = delete;

            ~TaskData() {
                CleanupContext();
            }

            void CleanupContext() {
                switch (contextType) {
                case SDContext:
                    if (sdContext) {
                        if (contextAcquired && !modelLoading)
                            Utils::SDContextManager::ReleaseContext(sdContext);
                        else
                            Utils::SDContextManager::ForceFreeContext(sdContext);
                        sdContext = nullptr;
                    }
                    break;
                case UpscalerContext:
                    if (upscalerContext) {
                        free_upscaler_ctx(upscalerContext);
                        upscalerContext = nullptr;
                    }
                    break;
                default: break;
                }
                contextType = NoContext;
                contextAcquired = false;
                modelLoading = false;
            }
        };

        SDCPPSystem(EntityManager& entityMgr)
            : BaseSystem(entityMgr), pauseWorker(false), hasActiveTask(false), clearRequested(false) {
            sysName = "SDCPPSystem";
        }

        ~SDCPPSystem() { Shutdown(); }

        void Shutdown() {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                shuttingDown = true;
                pauseWorker = true;
            }
            StopCurrentTask();
            ClearQueue();
            if (workerThread.joinable())
                workerThread.join();
            if (m_threadPool) {
                m_threadPool->terminateAll();
            }
            Utils::SDContextManager::ClearAllContexts();
        }

        void TerminateImmediately() {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                shuttingDown = true;
                pauseWorker = true;
                terminateFlag = true;
            }
            ClearQueue();
            Utils::SDContextManager::ClearAllContexts();
            if (m_threadPool) {
                m_threadPool->terminateAll();
            }
        }

        void QueueTask(EntityID entityID, TaskType taskType) {
            if (!mgr.IsEntityValid(entityID)) {
                std::cerr << "[QueueTask] Invalid entity\n";
                return;
            }
            std::lock_guard<std::mutex> lock(queueMutex);
            if (shuttingDown) return;
            TaskData taskData;
            taskData.entityID = entityID;
            taskData.processing = false;
            taskData.taskType = taskType;
            taskData.modelLoading = false;
            taskData.contextType = TaskData::NoContext;

            if (taskType == TaskType::Inference || taskType == TaskType::Img2Img ||
                taskType == TaskType::Img2Vid || taskType == TaskType::Edit) {
                if (mgr.HasComponent<SamplerComponent>(entityID)) {
                    auto& sampler = mgr.GetComponent<SamplerComponent>(entityID);
                    if (sampler.seed < 0) {
                        sampler.seed = static_cast<int>(Utils::generateRandomSeed());
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
            CheckModelLoadingFailures();
        }

        void RemoveFromQueue(size_t index) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (index < taskQueue.size() && !taskQueue[index].processing) {
                taskQueue[index].CleanupContext();
                entitiesNeedingCleanup.push_back(taskQueue[index].entityID);
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
            if (hasActiveTask && activeThreadId != std::thread::id{}) {
                terminateFlag = true;
            }
        }

        void ClearQueue() { clearRequested = true; }
        void PauseWorker() { std::lock_guard<std::mutex> lock(queueMutex); pauseWorker = true; }
        void ResumeWorker() { std::lock_guard<std::mutex> lock(queueMutex); pauseWorker = false; }

        size_t GetNumThreads() const {
            return m_threadPool ? m_threadPool->getDiffusionPool().size() : 0;
        }
        size_t GetQueuedTaskCount() const {
            return m_threadPool ? m_threadPool->getDiffusionPool().queueSize() : 0;
        }
        size_t GetActiveTaskCount() const {
            return m_threadPool ? m_threadPool->getDiffusionPool().activeCount() : 0;
        }
        bool HasActiveTask() const { std::lock_guard<std::mutex> lock(queueMutex); return hasActiveTask; }
        std::thread::id GetActiveThreadId() const { std::lock_guard<std::mutex> lock(queueMutex); return activeThreadId; }
        size_t GetQueueSize() const { std::lock_guard<std::mutex> lock(queueMutex); return taskQueue.size(); }
        EntityID GetLastGeneratedVideo() const { std::lock_guard<std::mutex> lock(queueMutex); return lastGeneratedVideoEntity; }

        void ClearAllSDContexts() { Utils::SDContextManager::ClearAllContexts(); }
        void ListSDContexts() { Utils::SDContextManager::ListCachedContexts(); }
        std::tuple<size_t, size_t, size_t> GetSDContextStats() {
            size_t total, inUse, available;
            Utils::SDContextManager::GetCacheStats(total, inUse, available);
            return std::make_tuple(total, inUse, available);
        }
        std::tuple<size_t, size_t> GetModelLoadingStats() {
            size_t loading, failed;
            Utils::SDContextManager::GetLoadingStats(loading, failed);
            return std::make_tuple(loading, failed);
        }
        void ForceModelReload() { std::lock_guard<std::mutex> lock(queueMutex); forceModelReload = true; }
        void SetMaxModelCache(size_t maxModels) { Utils::SDContextManager::SetMaxCacheSize(maxModels); }
        size_t GetMaxModelCache() const { return Utils::SDContextManager::GetMaxCacheSize(); }
        void UnloadModel(const std::string& modelPath) {
            std::lock_guard<std::mutex> lock(queueMutex);
            bool inUse = false;
            for (const auto& task : taskQueue) {
                if (task.processing && task.sdContext) { inUse = true; break; }
            }
            if (!inUse) Utils::SDContextManager::UnloadSpecificModel(modelPath);
        }
        void UnloadAllModels() {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (!hasActiveTask) Utils::SDContextManager::UnloadAllModels();
        }
        void ForceUnloadIdleModels() {
            Utils::SDContextManager::UnloadAllModels();
        }
        bool IsModelLoaded(const std::string& modelPath) const { return Utils::SDContextManager::IsModelLoaded(modelPath); }
        std::vector<std::string> GetLoadedModels() const { return Utils::SDContextManager::GetLoadedModels(); }
        std::tuple<size_t, size_t, size_t> GetModelCacheStats() const {
            size_t total, inUse, available;
            Utils::SDContextManager::GetCacheStats(total, inUse, available);
            return std::make_tuple(total, inUse, available);
        }

        void Start() override {
            m_threadPool = mgr.GetSystem<ThreadPoolSystem>();
            if (!m_threadPool) {
                std::cerr << "[SDCPPSystem] ThreadPoolSystem not available\n";
            }
            workerThread = std::thread([this]() { WorkerThread(); });
        }

        void Destroy() override {
            Shutdown();
            BaseSystem::Destroy();
        }

    private:
        std::vector<TaskData> taskQueue;
        std::atomic<bool> pauseWorker{ false };
        std::atomic<bool> shuttingDown{ false };
        std::atomic<bool> terminateFlag{ false };
        std::atomic<bool> clearRequested{ false };
        std::atomic<bool> forceModelReload{ false };
        std::vector<EntityID> entitiesNeedingCleanup;
        mutable std::mutex queueMutex;
        EntityID lastGeneratedVideoEntity{ 0 };
        std::thread workerThread;
        bool hasActiveTask{ false };
        std::thread::id activeThreadId{};
        std::shared_ptr<ThreadPoolSystem> m_threadPool;

        TaskData::ContextType GetRequiredContextType(TaskType taskType) {
            switch (taskType) {
            case TaskType::Inference:
            case TaskType::Img2Img:
            case TaskType::Img2Vid:
            case TaskType::Edit:
                return TaskData::SDContext;
            case TaskType::Upscaling:
                return TaskData::UpscalerContext;
            default:
                return TaskData::NoContext;
            }
        }

        bool IsTaskReadyForProcessing(TaskData& task) {
            switch (task.contextType) {
            case TaskData::SDContext:
                if (task.sdContext != nullptr && !task.modelLoading) return true;
                if (task.modelLoading) {
                    sd_ctx_t* loaded = Utils::SDContextManager::TryGetLoadedContext(task.metadata);
                    if (loaded) {
                        task.sdContext = loaded;
                        task.modelLoading = false;
                        task.contextAcquired = true;
                        return true;
                    }
                    return false;
                }
                return false;
            case TaskData::UpscalerContext:
                return task.upscalerContext != nullptr;
            case TaskData::NoContext:
                return true;
            default:
                return false;
            }
        }

        bool AcquireContextForTask(TaskData& task) {
            switch (task.contextType) {
            case TaskData::SDContext: {
                if (forceModelReload) {
                    task.sdContext = Utils::SDContextManager::CreateNewContext(task.metadata);
                    task.modelLoading = false;
                    forceModelReload = false;
                    task.contextAcquired = (task.sdContext != nullptr);
                    return task.contextAcquired;
                }
                task.sdContext = Utils::SDContextManager::GetOrCreateContext(task.metadata);
                if (!task.sdContext) {
                    task.modelLoading = true;
                    return false;
                }
                task.modelLoading = false;
                task.contextAcquired = true;
                return true;
            }
            case TaskData::UpscalerContext: {
                task.upscalerContext = CreateUpscalerContext(task.metadata);
                task.contextAcquired = (task.upscalerContext != nullptr);
                return task.contextAcquired;
            }
            default:
                task.contextAcquired = true;
                return true;
            }
        }

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

        void CheckModelLoadingFailures() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& task : taskQueue) {
                if (!task.processing && task.modelLoading && task.contextType == TaskData::SDContext) {
                    if (!Utils::SDContextManager::IsContextLoading(task.metadata)) {
                        sd_ctx_t* loaded = Utils::SDContextManager::TryGetLoadedContext(task.metadata);
                        if (loaded == nullptr) {
                            task.modelLoading = false;
                            task.contextAcquired = false;
                        }
                        else {
                            task.sdContext = loaded;
                            task.modelLoading = false;
                            task.contextAcquired = true;
                        }
                    }
                }
            }
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
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    lastGeneratedVideoEntity = videoEntity;
                }
            }
        }

        void HandleClearRequest() {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (auto& task : taskQueue)
                if (!task.processing) task.CleanupContext();
            taskQueue.erase(std::remove_if(taskQueue.begin(), taskQueue.end(),
                [](const TaskData& t) { return !t.processing; }), taskQueue.end());
        }

        template<typename Func, typename... Args>
        auto CreateTaskWrapper(EntityID entityID, Func&& func, Args&&... args) {
            return [this, entityID, func = std::forward<Func>(func), args...]() -> bool {
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    hasActiveTask = true;
                    activeThreadId = std::this_thread::get_id();
                    terminateFlag = false;
                }
                bool result = false;
                try {
                    result = func(args...);
                }
                catch (...) {}
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    hasActiveTask = false;
                    activeThreadId = std::thread::id{};
                    terminateFlag = false;
                }
                return result;
                };
        }

        static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            if (Utils::SDContextManager::IsContextLoading(metadata)) return false;
            return Utils::Txt2Img::RunInference(metadata, fullPath, context);
        }

        static bool RunConversion(const nlohmann::json& metadata) {
            return Utils::Conversion::ConvertToGGUF(metadata);
        }

        static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            if (Utils::SDContextManager::IsContextLoading(metadata)) return false;
            nlohmann::json modified = metadata;
            if (modified.contains("components") && modified["components"].is_array()) {
                for (auto& comp : modified["components"]) {
                    if (comp.contains("Vae")) {
                        comp["Vae"]["vae_decode_only"] = false;
                    }
                }
            }
            return Utils::Img2Img::RunImg2Img(modified, fullPath, context);
        }

        static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            if (Utils::SDContextManager::IsContextLoading(metadata)) return false;
            nlohmann::json modified = metadata;
            if (modified.contains("components") && modified["components"].is_array()) {
                for (auto& comp : modified["components"]) {
                    if (comp.contains("Vae")) {
                        comp["Vae"]["vae_decode_only"] = false;
                    }
                }
            }
            return Utils::Img2Vid::RunImg2Vid(modified, fullPath, context);
        }

        static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context) {
            if (Utils::SDContextManager::IsContextLoading(metadata)) return false;
            nlohmann::json modified = metadata;
            if (modified.contains("components") && modified["components"].is_array()) {
                for (auto& comp : modified["components"]) {
                    if (comp.contains("Vae")) {
                        comp["Vae"]["vae_decode_only"] = false;
                    }
                }
            }
            return Utils::Edit::RunEdit(modified, fullPath, context);
        }

        static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath) {
            return Utils::Upscaling::RunUpscaling(metadata, fullPath);
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
            if (!m_threadPool) return;

            auto& diffusionPool = m_threadPool->getDiffusionPool();

            for (auto& task : taskQueue) {
                if (task.processing) continue;
                if (task.contextType == TaskData::NoContext)
                    task.contextType = GetRequiredContextType(task.taskType);

                if (!IsTaskReadyForProcessing(task)) {
                    if (!task.contextAcquired && !task.modelLoading) {
                        bool acquired = AcquireContextForTask(task);
                        if (!acquired) break;
                    }
                    else if (task.modelLoading) {
                        break;
                    }
                    else {
                        task.CleanupContext();
                        entitiesNeedingCleanup.push_back(task.entityID);
                        auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
                            [&task](const TaskData& t) { return t.entityID == task.entityID; });
                        if (it != taskQueue.end()) taskQueue.erase(it);
                        break;
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
                        std::string defaultProject = Utils::g_FilePathSystem ? Utils::g_FilePathSystem->GetPath("DefaultProject") : "";
                        if (!defaultProject.empty()) outputDir = defaultProject;
                    }
                    task.fullPath = Utils::PngMetadata::CreateUniqueFilename(fullFileName, outputDir);
                }

                try {
                    switch (task.taskType) {
                    case TaskType::Inference:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunInference, task.metadata, task.fullPath, task.sdContext)
                        );
                        break;
                    case TaskType::Conversion:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunConversion, task.metadata)
                        );
                        break;
                    case TaskType::Img2Img:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunImg2Img, task.metadata, task.fullPath, task.sdContext)
                        );
                        break;
                    case TaskType::Img2Vid:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunImg2Vid, task.metadata, task.fullPath, task.sdContext)
                        );
                        break;
                    case TaskType::Edit:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunEdit, task.metadata, task.fullPath, task.sdContext)
                        );
                        break;
                    case TaskType::Upscaling:
                        task.result = diffusionPool.submit(
                            CreateTaskWrapper(task.entityID, RunUpscaling, task.metadata, task.fullPath)
                        );
                        break;
                    default:
                        task.CleanupContext();
                        continue;
                    }
                    task.processing = true;
                    break;
                }
                catch (...) {
                    task.CleanupContext();
                    entitiesNeedingCleanup.push_back(task.entityID);
                    auto it = std::find_if(taskQueue.begin(), taskQueue.end(),
                        [&task](const TaskData& t) { return t.entityID == task.entityID; });
                    if (it != taskQueue.end()) taskQueue.erase(it);
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
                    if (it->processing && it->result.valid()) {
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
                            it->CleanupContext();
                            if (!shuttingDown && success) {
                                completedTasks.emplace_back(fullPath, taskType, entityID);
                            }
                            else {
                                if (std::filesystem::exists(fullPath))
                                    std::filesystem::remove(fullPath);
                            }
                            it = taskQueue.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }
                    else {
                        ++it;
                    }
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
}