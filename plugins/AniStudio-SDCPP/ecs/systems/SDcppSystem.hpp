// SDCPPSystem.hpp
#pragma once

#include "DiffusionOptions.hpp"
#include "ECS.h"
#include "rng.hpp"
#include "SDCPPComponents.h"
#include "AniEngineComponents.hpp"
#include "SDCPPUtils.hpp"
#include "SDCPPParamFill.hpp"
#include "SDContextHandle.hpp"
#include "PngMetadataUtils.hpp"
#include "ImageSystem.hpp"
#include "VideoSystem.hpp"
#include "AVSystem.hpp"
#include "pch.h"
#include "stable-diffusion.h"
#include "ThreadPoolSystem.hpp"
#include "SettingsSystem.hpp"
#include "FilePathSystem.hpp"
#include "ModelCacheSystem.hpp"

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
#include <utility>

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
            TaskType taskType = TaskType::Inference;
        };

        struct TaskData {
            EntityID entityID = 0;
            bool processing = false;
            bool cancelled = false;
            TaskType taskType = TaskType::Inference;

            std::shared_ptr<SDCPP::ResourceManager> genRes;
            sd_img_gen_params_t imgParams{};
            sd_vid_gen_params_t vidParams{};

            sd_ctx_params_t convParams{};

            nlohmann::json metadataForWrite;

            std::string fullPath;
            std::future<bool> result;

            std::shared_ptr<SDCPP::SDContextHandle> ctxHandle;
            std::shared_ptr<SDCPP::UpscalerHandle>  upscalerHandle;

            std::chrono::steady_clock::time_point enqueueTime;
            std::chrono::steady_clock::time_point startTime;
            std::chrono::steady_clock::time_point cancelTime;

            TaskData() = default;
            TaskData(TaskData&&) noexcept = default;
            TaskData& operator=(TaskData&&) noexcept = default;
            TaskData(const TaskData&) = delete;
            TaskData& operator=(const TaskData&) = delete;
            ~TaskData() = default;

            void Cancel();
        };

        explicit SDCPPSystem(EntityManager& entityMgr);
        ~SDCPPSystem() override;

        void Shutdown();
        void TerminateImmediately();

        void QueueTask(EntityID entityID, TaskType taskType);
        void Update(float deltaT) override;

        void RemoveFromQueue(size_t index);
        void MoveInQueue(size_t fromIndex, size_t toIndex);
        std::vector<QueueItem> GetQueueSnapshot();

        void StopCurrentTask();
        void CancelCurrentTask();
        void ClearQueuedTasks();
        void ClearAllTasks();

        void PauseWorker();
        void ResumeWorker();
        bool IsPaused() const;

        size_t GetNumThreads() const;
        size_t GetQueuedTaskCount() const;
        size_t GetActiveTaskCount() const;
        bool HasActiveTask() const;
        size_t GetQueueSize() const;

        void Start() override;
        void Destroy() override;

        std::vector<std::pair<TaskType, nlohmann::json>> GetQueueTasksWithMetadata() const;
        void QueueTaskFromSerialized(const nlohmann::json& entityData, TaskType taskType);

    private:
        std::vector<TaskData> taskQueue;
        std::atomic<bool> pauseWorker{ false };
        std::atomic<bool> shuttingDown{ false };
        std::atomic<bool> clearRequested{ false };
        mutable std::mutex queueMutex;
        std::thread workerThread;
        bool hasActiveTask{ false };
        std::thread::id activeThreadId{};
        std::shared_ptr<ThreadPoolSystem> m_threadPool;
        std::shared_ptr<FilePathSystem> m_filePathSystem;
        std::shared_ptr<ModelCacheSystem> m_cacheSystem;

        std::string ResolveOutputDirectory(const std::string& raw);
        std::string ResolveFullPathForTask(const TaskData& task);

        void LoadImageViaImageSystem(EntityID targetEntity, const std::string& filePath);
        void LoadVideoViaVideoSystem(EntityID targetEntity, const std::string& filePath);
        EntityID LoadVideoWithAudio(const std::string& filePath);
        void HandleClearRequest();

        static bool RunInference(const sd_img_gen_params_t& params,
            const nlohmann::json& metadataForWrite,
            const std::string& fullPath,
            sd_ctx_t* context);
        static bool RunImg2Img(const sd_img_gen_params_t& params,
            const nlohmann::json& metadataForWrite,
            const std::string& fullPath,
            sd_ctx_t* context);
        static bool RunImg2Vid(const sd_vid_gen_params_t& params,
            const nlohmann::json& metadataForWrite,
            const std::string& fullPath,
            sd_ctx_t* context);
        static bool RunEdit(const sd_img_gen_params_t& params,
            const nlohmann::json& metadataForWrite,
            const std::string& fullPath,
            sd_ctx_t* context);
        static bool RunUpscaling(const nlohmann::json& metadataForWrite,
            const std::string& fullPath,
            upscaler_ctx_t* upscaler,
            const std::string& inputImagePath,
            uint32_t upscaleFactor);
        static bool RunConversion(const sd_ctx_params_t& ctx);

        bool IsVideoTask(TaskType taskType) const;
        std::string GetOutputExtension(TaskType taskType, EntityID entityID) const;

        void ProcessQueues();
        void CheckTaskCompletion();
        void ProcessCompletedTask(const std::string& fullPath, TaskType taskType, EntityID entityID);
        void WorkerThread();
    };

} // namespace ECS