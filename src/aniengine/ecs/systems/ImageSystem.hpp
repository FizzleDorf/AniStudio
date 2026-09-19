#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "ImageComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include "ImageUtils.hpp"
#include <memory>
#include <functional>
#include <stb_image.h>
#include <stb_image_write.h>
#include <queue>
#include <mutex>
#include <future>
#include <chrono>
#include <filesystem>
#include <vector>
#include <utility>

namespace ECS {

    class ImageSystem : public BaseSystem {
    public:
        using ImageCallback = std::function<void(EntityID)>;
        using ImageReadyCallback = std::function<void(EntityID, unsigned char*, int, int, int)>;

        struct LoadResult {
            bool success = false;
            unsigned char* data = nullptr;
            int width = 0;
            int height = 0;
            int channels = 0;
            std::string fileName;
            std::string filePath;
            EntityID entityID = 0;
            uint64_t fileSize = 0;
            std::string fileDate;
            std::string fileTime;
            bool hasExif = false;
            bool hasLSB = false;
            bool hasAniStudio = false;

            LoadResult() = default;
            LoadResult(const LoadResult&) = delete;
            LoadResult& operator=(const LoadResult&) = delete;
            LoadResult(LoadResult&& other) noexcept;
            LoadResult& operator=(LoadResult&& other) noexcept;
            ~LoadResult();
        };

        struct LoadingTask {
            EntityID entityID;
            std::string filePath;
            std::future<LoadResult> future;

            LoadingTask() = default;
            LoadingTask(LoadingTask&& other) noexcept;
            LoadingTask& operator=(LoadingTask&& other) noexcept;
            LoadingTask(const LoadingTask&) = delete;
            LoadingTask& operator=(const LoadingTask&) = delete;
        };

        ImageSystem(EntityManager& entityMgr);
        ~ImageSystem() override;

        void Start() override;
        void Update(const float deltaT) override;

        void RegisterImageAddedCallback(void* owner, const ImageCallback& callback);
        void RegisterImageRemovedCallback(void* owner, const ImageCallback& callback);
        void RegisterImageReadyCallback(void* owner, const ImageReadyCallback& callback);
        void UnregisterCallbacksForOwner(void* owner);

        void SetImage(const EntityID entity, const std::string& filePath);
        void RemoveImage(const EntityID entity);
        std::vector<EntityID> GetAllImageEntities() const;

    private:
        std::vector<std::pair<void*, ImageCallback>> imageAddedCallbacks;
        std::vector<std::pair<void*, ImageCallback>> imageRemovedCallbacks;
        std::vector<std::pair<void*, ImageReadyCallback>> imageReadyCallbacks;
        std::vector<LoadingTask> pendingLoads;
        mutable std::mutex loadMutex;

        void LoadImageAsync(EntityID entity, const std::string& filePath);
        void ProcessCompletedLoads();
        void NotifyImageAdded(EntityID entity);
        void NotifyImageRemoved(EntityID entity);
        void NotifyImageReady(EntityID entity, unsigned char* data, int w, int h, int ch);
    };

} // namespace ECS