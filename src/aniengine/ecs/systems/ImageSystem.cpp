#include "ImageSystem.hpp"
#include "ThreadPoolSystem.hpp"
#include "ImageUtils.hpp"
#include "Log.hpp"
#include <stb_image.h>
#include <algorithm>

namespace ECS {

    ImageSystem::LoadResult::LoadResult(LoadResult&& other) noexcept
        : success(other.success)
        , data(other.data)
        , width(other.width)
        , height(other.height)
        , channels(other.channels)
        , fileName(std::move(other.fileName))
        , filePath(std::move(other.filePath))
        , entityID(other.entityID)
        , fileSize(other.fileSize)
        , fileDate(std::move(other.fileDate))
        , fileTime(std::move(other.fileTime))
        , hasExif(other.hasExif)
        , hasLSB(other.hasLSB)
        , hasAniStudio(other.hasAniStudio) {
        other.data = nullptr;
    }

    ImageSystem::LoadResult& ImageSystem::LoadResult::operator=(LoadResult&& other) noexcept {
        if (this != &other) {
            if (data) {
                stbi_image_free(data);
            }
            success = other.success;
            data = other.data;
            width = other.width;
            height = other.height;
            channels = other.channels;
            fileName = std::move(other.fileName);
            filePath = std::move(other.filePath);
            entityID = other.entityID;
            fileSize = other.fileSize;
            fileDate = std::move(other.fileDate);
            fileTime = std::move(other.fileTime);
            hasExif = other.hasExif;
            hasLSB = other.hasLSB;
            hasAniStudio = other.hasAniStudio;
            other.data = nullptr;
        }
        return *this;
    }

    ImageSystem::LoadResult::~LoadResult() {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
    }

    ImageSystem::LoadingTask::LoadingTask(LoadingTask&& other) noexcept
        : entityID(other.entityID)
        , filePath(std::move(other.filePath))
        , future(std::move(other.future)) {
    }

    ImageSystem::LoadingTask& ImageSystem::LoadingTask::operator=(LoadingTask&& other) noexcept {
        if (this != &other) {
            entityID = other.entityID;
            filePath = std::move(other.filePath);
            future = std::move(other.future);
        }
        return *this;
    }

    ImageSystem::ImageSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "ImageSystem";
        AddComponentSignature<ImageComponent>();
    }

    ImageSystem::~ImageSystem() {
        ANI_LOG_DEBUG("[ImageSystem] Destructor - cleaning up");
        std::lock_guard<std::mutex> lock(loadMutex);
        for (auto& task : pendingLoads) {
            if (task.future.valid()) {
                try {
                    if (task.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        task.future.get();
                    }
                }
                catch (...) {}
            }
        }
        pendingLoads.clear();
        imageAddedCallbacks.clear();
        imageRemovedCallbacks.clear();
        imageReadyCallbacks.clear();
    }

    void ImageSystem::Start() {
        ANI_LOG_INFO("[ImageSystem] Started");
        for (auto entity : entities) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
                if (!imageComp.filePath.empty()) {
                    LoadImageAsync(entity, imageComp.filePath);
                }
            }
        }
    }

    void ImageSystem::Update(const float deltaT) {
        ProcessCompletedLoads();
    }

    void ImageSystem::RegisterImageAddedCallback(void* owner, const ImageCallback& callback) {
        imageAddedCallbacks.emplace_back(owner, callback);
    }

    void ImageSystem::RegisterImageRemovedCallback(void* owner, const ImageCallback& callback) {
        imageRemovedCallbacks.emplace_back(owner, callback);
    }

    void ImageSystem::RegisterImageReadyCallback(void* owner, const ImageReadyCallback& callback) {
        imageReadyCallbacks.emplace_back(owner, callback);
    }

    void ImageSystem::UnregisterCallbacksForOwner(void* owner) {
        imageAddedCallbacks.erase(
            std::remove_if(imageAddedCallbacks.begin(), imageAddedCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            imageAddedCallbacks.end());
        imageRemovedCallbacks.erase(
            std::remove_if(imageRemovedCallbacks.begin(), imageRemovedCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            imageRemovedCallbacks.end());
        imageReadyCallbacks.erase(
            std::remove_if(imageReadyCallbacks.begin(), imageReadyCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            imageReadyCallbacks.end());
    }

    void ImageSystem::SetImage(const EntityID entity, const std::string& filePath) {
        if (mgr.HasComponent<ImageComponent>(entity)) {
            auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
            imageComp.ClearImageData();

            if (mgr.HasComponent<InputImageComponent>(entity)) {
                auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
                inputComp.ClearImageData();
            }
            LoadImageAsync(entity, filePath);
        }
    }

    void ImageSystem::RemoveImage(const EntityID entity) {
        if (!mgr.HasComponent<ImageComponent>(entity)) return;

        NotifyImageRemoved(entity);

        if (mgr.HasComponent<InputImageComponent>(entity)) {
            mgr.GetComponent<InputImageComponent>(entity).ClearImageData();
        }
        mgr.GetComponent<ImageComponent>(entity).ClearImageData();
    }

    std::vector<EntityID> ImageSystem::GetAllImageEntities() const {
        std::vector<EntityID> result;
        for (auto entity : entities) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                result.push_back(entity);
            }
        }
        return result;
    }

    void ImageSystem::LoadImageAsync(EntityID entity, const std::string& filePath) {
        auto threadPoolSys = mgr.GetSystem<ThreadPoolSystem>();
        if (!threadPoolSys) {
            ANI_LOG_ERROR("[ImageSystem] ThreadPoolSystem not available!");
            return;
        }

        auto& ioPool = threadPoolSys->getIOPool();

        auto future = ioPool.submit([filePath, entity]() -> LoadResult {
            LoadResult result;
            result.filePath = filePath;
            result.entityID = entity;

            size_t lastSlash = filePath.find_last_of("/\\");
            result.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

            result.data = Utils::ImageUtils::LoadImageData(filePath, result.width, result.height, result.channels);
            result.success = (result.data != nullptr);

            if (result.success) {
                try {
                    result.fileSize = std::filesystem::file_size(filePath);
                    auto ftime = std::filesystem::last_write_time(filePath);
                    auto now = std::chrono::system_clock::now();
                    auto diff = ftime - std::filesystem::file_time_type::clock::now();
                    auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
                    std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
                    std::tm tm = *std::localtime(&tt);
                    char buf[32];
                    strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
                    result.fileDate = buf;
                    strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
                    result.fileTime = buf;
                }
                catch (...) {}

                result.hasExif = Utils::ImageUtils::HasExifMetadata(filePath);
                result.hasLSB = Utils::ImageUtils::HasLSBMetadata(filePath);
                int status = Utils::ImageUtils::GetMetadataStatus(filePath);
                result.hasAniStudio = (status > 0);
            }
            return result;
            });

        std::lock_guard<std::mutex> lock(loadMutex);
        LoadingTask task;
        task.entityID = entity;
        task.filePath = filePath;
        task.future = std::move(future);
        pendingLoads.push_back(std::move(task));
    }

    void ImageSystem::ProcessCompletedLoads() {
        std::lock_guard<std::mutex> lock(loadMutex);

        for (auto it = pendingLoads.begin(); it != pendingLoads.end();) {
            if (it->future.valid() &&
                it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {

                try {
                    LoadResult result = it->future.get();

                    if (mgr.HasComponent<ImageComponent>(result.entityID)) {
                        auto& imageComp = mgr.GetComponent<ImageComponent>(result.entityID);

                        if (result.success) {
                            imageComp.SetImageData(result.data, result.width, result.height, result.channels);
                            result.data = nullptr;
                            imageComp.fileName = result.fileName;
                            imageComp.filePath = result.filePath;
                            imageComp.fileSize = result.fileSize;
                            imageComp.fileDate = result.fileDate;
                            imageComp.fileTime = result.fileTime;
                            imageComp.hasExifData = result.hasExif;
                            imageComp.hasLSBData = result.hasLSB;
                            imageComp.hasAniStudioMetadata = result.hasAniStudio;

                            if (mgr.HasComponent<InputImageComponent>(result.entityID)) {
                                auto& inputComp = mgr.GetComponent<InputImageComponent>(result.entityID);
                                inputComp.SetImageData(imageComp.imageData, result.width, result.height, result.channels);
                                inputComp.fileName = result.fileName;
                                inputComp.filePath = result.filePath;
                                inputComp.fileSize = result.fileSize;
                                inputComp.fileDate = result.fileDate;
                                inputComp.fileTime = result.fileTime;
                                inputComp.hasExifData = result.hasExif;
                                inputComp.hasLSBData = result.hasLSB;
                                inputComp.hasAniStudioMetadata = result.hasAniStudio;
                            }

                            NotifyImageReady(result.entityID, imageComp.imageData,
                                imageComp.width, imageComp.height, imageComp.channels);
                            NotifyImageAdded(result.entityID);
                        }
                        else {
                            ANI_LOG_ERROR("[ImageSystem] Failed to load image: %s", result.filePath.c_str());
                        }
                    }
                    else {
                        ANI_LOG_DEBUG("[ImageSystem] Entity %llu no longer has ImageComponent, freed data",
                            static_cast<unsigned long long>(result.entityID));
                    }
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[ImageSystem] Exception in ProcessCompletedLoads: %s", e.what());
                }

                it = pendingLoads.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void ImageSystem::NotifyImageAdded(EntityID entity) {
        for (const auto& [owner, cb] : imageAddedCallbacks) {
            (void)owner;
            try {
                cb(entity);
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[ImageSystem] Exception in image added callback: %s", e.what());
            }
        }
    }

    void ImageSystem::NotifyImageRemoved(EntityID entity) {
        for (const auto& [owner, cb] : imageRemovedCallbacks) {
            (void)owner;
            try {
                cb(entity);
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[ImageSystem] Exception in image removed callback: %s", e.what());
            }
        }
    }

    void ImageSystem::NotifyImageReady(EntityID entity, unsigned char* data, int w, int h, int ch) {
        for (const auto& [owner, cb] : imageReadyCallbacks) {
            (void)owner;
            try {
                cb(entity, data, w, h, ch);
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[ImageSystem] Exception in image ready callback: %s", e.what());
            }
        }
    }

} // namespace ECS