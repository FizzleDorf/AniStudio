#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "VideoUtils.hpp"
#include "Components.h"
#include "ThreadPoolSystem.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <future>
#include <mutex>
#include <shared_mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

namespace ECS {

    class VideoSystem : public BaseSystem {
    public:
        using VideoCallback = std::function<void(EntityID)>;
        using VideoTextureCallback = std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)>;
        using SaveCallback = std::function<void(EntityID, bool, const std::string&)>;
        using LoadCallback = std::function<void(EntityID, bool)>;

        VideoSystem(EntityManager& entityMgr);
        ~VideoSystem() override;

        void Start() override;
        void Update(float deltaT) override;

        void SetVideo(EntityID entity, const std::string& filePath);
        void RemoveVideo(EntityID entity);
        void ClearCache(EntityID entity);
        std::vector<EntityID> GetAllVideoEntities() const;

        void RegisterVideoAddedCallback(const VideoCallback& cb);
        void RegisterVideoRemovedCallback(const VideoCallback& cb);
        void RegisterSaveCallback(const SaveCallback& cb);
        void RegisterLoadCallback(const LoadCallback& cb);

        void SetVideoTextureCallback(const VideoTextureCallback& callback);

        bool IsLoading(EntityID entity) const;
        bool IsSaving(EntityID entity) const;

        void SaveVideoAsync(EntityID entity, const std::string& outputPath = "");

        bool DecodeFrameForSave(VideoComponent& videoComp, long long frameIndex, bool& isNewFrame);

        struct LoadResult {
            bool success = false;
            EntityID entityID = 0;
            std::string filePath;
            std::string fileName;
            int width = 0;
            int height = 0;
            double fps = 0.0;
            long long frameCount = 0;
            std::vector<uint8_t> firstFrameRGBA;
            uint64_t fileSize = 0;
            std::string fileDate;
            std::string fileTime;
            bool hasExif = false;
            bool hasLSB = false;
            bool hasAniStudio = false;
            AVFormatContext* fmtCtx = nullptr;
            AVCodecContext* codecCtx = nullptr;
            SwsContext* swsCtx = nullptr;
            AVFrame* frame = nullptr;
            AVPacket* pkt = nullptr;
            int videoStreamIndex = -1;

            ~LoadResult() {
                if (fmtCtx) avformat_close_input(&fmtCtx);
                if (codecCtx) avcodec_free_context(&codecCtx);
                if (swsCtx) sws_freeContext(swsCtx);
                if (frame) av_frame_free(&frame);
                if (pkt) av_packet_free(&pkt);
            }

            LoadResult() = default;
            LoadResult(LoadResult&& other) noexcept
                : success(other.success), entityID(other.entityID), filePath(std::move(other.filePath)),
                fileName(std::move(other.fileName)), width(other.width), height(other.height),
                fps(other.fps), frameCount(other.frameCount), firstFrameRGBA(std::move(other.firstFrameRGBA)),
                fileSize(other.fileSize), fileDate(std::move(other.fileDate)), fileTime(std::move(other.fileTime)),
                hasExif(other.hasExif), hasLSB(other.hasLSB), hasAniStudio(other.hasAniStudio),
                fmtCtx(other.fmtCtx), codecCtx(other.codecCtx), swsCtx(other.swsCtx),
                frame(other.frame), pkt(other.pkt), videoStreamIndex(other.videoStreamIndex) {
                other.fmtCtx = nullptr;
                other.codecCtx = nullptr;
                other.swsCtx = nullptr;
                other.frame = nullptr;
                other.pkt = nullptr;
            }

            LoadResult& operator=(LoadResult&& other) noexcept {
                if (this != &other) {
                    if (fmtCtx) avformat_close_input(&fmtCtx);
                    if (codecCtx) avcodec_free_context(&codecCtx);
                    if (swsCtx) sws_freeContext(swsCtx);
                    if (frame) av_frame_free(&frame);
                    if (pkt) av_packet_free(&pkt);

                    success = other.success;
                    entityID = other.entityID;
                    filePath = std::move(other.filePath);
                    fileName = std::move(other.fileName);
                    width = other.width;
                    height = other.height;
                    fps = other.fps;
                    frameCount = other.frameCount;
                    firstFrameRGBA = std::move(other.firstFrameRGBA);
                    fileSize = other.fileSize;
                    fileDate = std::move(other.fileDate);
                    fileTime = std::move(other.fileTime);
                    hasExif = other.hasExif;
                    hasLSB = other.hasLSB;
                    hasAniStudio = other.hasAniStudio;
                    fmtCtx = other.fmtCtx;
                    codecCtx = other.codecCtx;
                    swsCtx = other.swsCtx;
                    frame = other.frame;
                    pkt = other.pkt;
                    videoStreamIndex = other.videoStreamIndex;

                    other.fmtCtx = nullptr;
                    other.codecCtx = nullptr;
                    other.swsCtx = nullptr;
                    other.frame = nullptr;
                    other.pkt = nullptr;
                }
                return *this;
            }

            LoadResult(const LoadResult&) = delete;
            LoadResult& operator=(const LoadResult&) = delete;
        };

    private:
        struct SaveTaskData {
            std::string inputPath;
            std::string outputPath;
            int fps = 24;
            int width = 0;
            int height = 0;
            long long frameCount = 0;
        };

        struct LoadingTask {
            EntityID entityID;
            std::string filePath;
            std::future<LoadResult> future;
            LoadingTask() = default;
            LoadingTask(LoadingTask&&) noexcept = default;
            LoadingTask& operator=(LoadingTask&&) noexcept = default;
            LoadingTask(const LoadingTask&) = delete;
            LoadingTask& operator=(const LoadingTask&) = delete;
        };

        std::vector<VideoCallback> videoAddedCallbacks;
        std::vector<VideoCallback> videoRemovedCallbacks;
        std::vector<SaveCallback> saveCallbacks;
        std::vector<LoadCallback> loadCallbacks;
        VideoTextureCallback m_textureCallback;

        std::unordered_map<EntityID, std::future<bool>> m_saveFutures;
        std::unordered_map<EntityID, std::string> m_savePaths;
        mutable std::mutex m_saveMutex;

        std::vector<LoadingTask> m_pendingLoads;
        mutable std::recursive_mutex m_loadMutex;
        std::unordered_map<EntityID, bool> m_loadingStatus;

        void LoadVideoAsync(EntityID entity, const std::string& filePath);
        void ProcessCompletedLoads();
        bool SaveVideoInBackground(const SaveTaskData& taskData);
        void ProcessCompletedSaves();

        void NotifyVideoAdded(EntityID entity);
        void NotifyVideoRemoved(EntityID entity);
        void NotifySaveComplete(EntityID entity, bool success, const std::string& path);
        void NotifyLoadComplete(EntityID entity, bool success);

        static LoadResult LoadVideoInBackground(const std::string& filePath, EntityID entity);
        void ApplyLoadedVideo(LoadResult&& result);
    };

}