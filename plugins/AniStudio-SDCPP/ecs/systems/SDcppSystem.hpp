// SDcppSystem.hpp
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
            std::string contextKey;
            std::chrono::steady_clock::time_point enqueueTime;
            std::chrono::steady_clock::time_point startTime;

            TaskData() = default;
            TaskData(TaskData&& other) noexcept;
            TaskData& operator=(TaskData&& other) noexcept;
            TaskData(const TaskData&) = delete;
            TaskData& operator=(const TaskData&) = delete;
            ~TaskData();

            void Cancel();
        };

        explicit SDCPPSystem(EntityManager& entityMgr);
        ~SDCPPSystem();

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
        std::vector<EntityID> entitiesNeedingCleanup;
        mutable std::mutex queueMutex;
        std::thread workerThread;
        bool hasActiveTask{ false };
        std::thread::id activeThreadId{};
        std::shared_ptr<ThreadPoolSystem> m_threadPool;
        std::shared_ptr<FilePathSystem> m_filePathSystem;
        std::shared_ptr<ModelCacheSystem> m_cacheSystem;

        upscaler_ctx_t* CreateUpscalerContext(const nlohmann::json& metadata);
        void LoadImageViaImageSystem(EntityID targetEntity, const std::string& filePath);
        void LoadVideoViaVideoSystem(const std::string& filePath);
        void HandleClearRequest();

        static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context);
        static bool RunImg2Img(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context);
        static bool RunImg2Vid(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context);
        static bool RunEdit(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* context);
        static bool RunUpscaling(const nlohmann::json& metadata, const std::string& fullPath, upscaler_ctx_t* upscaler);
        static bool RunConversion(const nlohmann::json& metadata);

        bool IsVideoTask(TaskType taskType) const;
        std::string GetOutputExtension(TaskType taskType) const;

        void ProcessQueues();
        void CheckTaskCompletion();
        void ProcessCompletedTask(const std::string& fullPath, TaskType taskType, EntityID entityID);
        void WorkerThread();
        int CountPendingTasksForContext(const std::string& contextKey);
    };

}