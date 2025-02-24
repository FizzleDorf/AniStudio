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

    void ClearQueue() {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (!inferenceQueue.empty()) {
            for (auto i = 0; i < inferenceQueue.size();) {
                if (!inferenceQueue[i].processing) {
                    inferenceQueue.erase(inferenceQueue.begin() + i);
                } else {
                    ++i;
                }
            }
        }
    }

    void PauseWorker() { pauseWorker.store(true); }

    void ResumeWorker() {
        pauseWorker.store(false);
        queueCondition.notify_all(); // Wake up the worker thread
    }