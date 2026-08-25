#include "ImageSystem.hpp"
#include "ThreadPoolSystem.hpp"
#include "ImageUtils.hpp"
#include <iostream>
#include <stb_image.h>

namespace ECS {

    ImageSystem::LoadResult::~LoadResult() {
        data = nullptr;
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
        std::cout << "[ImageSystem] Destructor - cleaning up" << std::endl;
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
    }

    void ImageSystem::Start() {
        std::cout << "[ImageSystem] Started" << std::endl;
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

    void ImageSystem::RegisterImageAddedCallback(const ImageCallback& callback) {
        imageAddedCallbacks.push_back(callback);
    }

    void ImageSystem::RegisterImageRemovedCallback(const ImageCallback& callback) {
        imageRemovedCallbacks.push_back(callback);
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
        if (mgr.HasComponent<ImageComponent>(entity)) {
            if (mgr.HasComponent<InputImageComponent>(entity)) {
                auto& inputComp = mgr.GetComponent<InputImageComponent>(entity);
                inputComp.ClearImageData();
            }
            else {
                auto& imageComp = mgr.GetComponent<ImageComponent>(entity);
                imageComp.ClearImageData();
            }
            NotifyImageRemoved(entity);
            mgr.DestroyEntity(entity);
        }
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
            std::cerr << "[ImageSystem] ThreadPoolSystem not available!" << std::endl;
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
                                inputComp.SetImageData(result.data, result.width, result.height, result.channels);
                                inputComp.fileName = result.fileName;
                                inputComp.filePath = result.filePath;
                                inputComp.fileSize = result.fileSize;
                                inputComp.fileDate = result.fileDate;
                                inputComp.fileTime = result.fileTime;
                                inputComp.hasExifData = result.hasExif;
                                inputComp.hasLSBData = result.hasLSB;
                                inputComp.hasAniStudioMetadata = result.hasAniStudio;
                            }

                            NotifyImageAdded(result.entityID);
                        }
                        else {
                            std::cerr << "[ImageSystem] Failed to load image: " << result.filePath << std::endl;
                        }
                    }
                    else {
                        if (result.data) {
                            stbi_image_free(result.data);
                            result.data = nullptr;
                        }
                        std::cout << "[ImageSystem] Entity " << result.entityID << " no longer has ImageComponent, freed data" << std::endl;
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "[ImageSystem] Exception in ProcessCompletedLoads: " << e.what() << std::endl;
                }

                it = pendingLoads.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void ImageSystem::NotifyImageAdded(EntityID entity) {
        for (const auto& cb : imageAddedCallbacks) {
            try {
                cb(entity);
            }
            catch (const std::exception& e) {
                std::cerr << "[ImageSystem] Exception in image added callback: " << e.what() << std::endl;
            }
        }
    }

    void ImageSystem::NotifyImageRemoved(EntityID entity) {
        for (const auto& cb : imageRemovedCallbacks) {
            try {
                cb(entity);
            }
            catch (const std::exception& e) {
                std::cerr << "[ImageSystem] Exception in image removed callback: " << e.what() << std::endl;
            }
        }
    }

} // namespace ECS