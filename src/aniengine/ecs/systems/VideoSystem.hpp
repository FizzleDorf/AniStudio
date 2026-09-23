#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "PlaybackStateComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <future>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace ECS {

    class AudioSystem;

    class VideoSystem : public BaseSystem {
    public:
        using VideoCallback = std::function<void(EntityID)>;
        using VideoTextureCallback =
            std::function<void(EntityID, const uint8_t*, int, int, int)>;
        using SaveCallback = std::function<void(EntityID, bool, const std::string&)>;
        using LoadCallback = std::function<void(EntityID, bool)>;
        using EndCallback = std::function<void(EntityID)>;

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

            LoadResult() = default;
            ~LoadResult() {
                if (fmtCtx) avformat_close_input(&fmtCtx);
                if (codecCtx) avcodec_free_context(&codecCtx);
                if (swsCtx) sws_free_context(&swsCtx);
                if (frame) av_frame_free(&frame);
                if (pkt) av_packet_free(&pkt);
            }
            LoadResult(const LoadResult&) = delete;
            LoadResult& operator=(const LoadResult&) = delete;
            LoadResult(LoadResult&& o) noexcept { *this = std::move(o); }
            LoadResult& operator=(LoadResult&& o) noexcept {
                if (this != &o) {
                    if (fmtCtx) avformat_close_input(&fmtCtx);
                    if (codecCtx) avcodec_free_context(&codecCtx);
                    if (swsCtx) sws_free_context(&swsCtx);
                    if (frame) av_frame_free(&frame);
                    if (pkt) av_packet_free(&pkt);
                    success = o.success; entityID = o.entityID;
                    filePath = std::move(o.filePath); fileName = std::move(o.fileName);
                    width = o.width; height = o.height; fps = o.fps; frameCount = o.frameCount;
                    firstFrameRGBA = std::move(o.firstFrameRGBA);
                    fileSize = o.fileSize;
                    fileDate = std::move(o.fileDate); fileTime = std::move(o.fileTime);
                    hasExif = o.hasExif; hasLSB = o.hasLSB; hasAniStudio = o.hasAniStudio;
                    fmtCtx = o.fmtCtx; codecCtx = o.codecCtx; swsCtx = o.swsCtx;
                    frame = o.frame; pkt = o.pkt; videoStreamIndex = o.videoStreamIndex;
                    o.fmtCtx = nullptr; o.codecCtx = nullptr; o.swsCtx = nullptr;
                    o.frame = nullptr; o.pkt = nullptr;
                }
                return *this;
            }
        };

        VideoSystem(EntityManager& entityMgr);
        ~VideoSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;
        void OnEntityDestroyed(EntityID entity) override;

        void SetAudioSystem(AudioSystem* audio) { m_audio = audio; }

        void LoadVideo(EntityID entity, const std::string& filePath,
            PlaybackMode mode = PlaybackMode::Cached);
        void SetVideo(EntityID entity, const std::string& filePath,
            PlaybackMode mode = PlaybackMode::Cached) {
            LoadVideo(entity, filePath, mode);
        }

        void RemoveVideo(EntityID entity);
        void ClearCache(EntityID entity);
        void SetMode(EntityID entity, PlaybackMode mode,
            double keepTime, bool wasPlaying);

        void Play(EntityID entity, bool loop = false);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetSpeed(EntityID entity, float speed);

        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;
        bool IsLoading(EntityID entity) const;
        bool IsSaving(EntityID entity) const;

        double GetCurrentPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;

        std::vector<EntityID> GetAllVideoEntities() const;

        bool GetCurrentFrame(EntityID entity,
            std::vector<uint8_t>& outData,
            int& outWidth,
            int& outHeight) const;

        void SaveVideoAsync(EntityID entity, const std::string& outputPath = "");

        bool DecodeFrameForSave(VideoComponent& videoComp, long long frameIndex, bool& isNewFrame);

        void RegisterVideoAddedCallback(void* owner, const VideoCallback& cb);
        void RegisterVideoRemovedCallback(void* owner, const VideoCallback& cb);
        void RegisterVideoTextureCallback(void* owner, const VideoTextureCallback& cb);
        void RegisterSaveCallback(void* owner, const SaveCallback& cb);
        void RegisterLoadCallback(void* owner, const LoadCallback& cb);
        void RegisterEndCallback(void* owner, const EndCallback& cb);
        void UnregisterCallbacksForOwner(void* owner);

    private:
        struct Frame {
            std::vector<uint8_t> data;
            int width = 0;
            int height = 0;
            double pts = 0.0;
            long long index = 0;
        };

        enum class CmdType { Seek, Pause, Resume, Stop, Shutdown };
        struct Cmd {
            CmdType type;
            double seekTime = 0.0;
        };

        struct Track {
            Track(VideoSystem* owner, EntityID e);
            ~Track();

            void StartThread();
            void Push(Cmd c);
            void ClearBuffer();

            bool TryPopFrameForTime(double targetTime, Frame& out, bool& got);
            bool TryPopAny(Frame& out);

            bool DecodeOneFrame(Frame& out);
            bool PerformSeek(double time);

            size_t BufferTarget() const;

            EntityID entity = 0;
            bool paused = true;
            bool stopped = true;
            bool endReached = false;
            bool loop = false;
            bool seeking = false;
            double pendingSeekTime = -1.0;
            double lastDisplayedPts = -1.0;
            long long lastDisplayedIndex = -1;
            bool firstFrameAfterSeek = false;

            PlaybackMode mode = PlaybackMode::Cached;

            std::vector<uint8_t> currentFrameRGBA;
            int currentWidth = 0;
            int currentHeight = 0;

            std::thread thread;
            std::atomic<bool> running{ true };
            std::atomic<bool> workerPaused{ true };

            std::deque<Cmd> commands;
            mutable std::mutex cmdMutex;
            std::condition_variable cmdCV;

            std::deque<Frame> buffer;
            mutable std::mutex bufferMutex;
            std::condition_variable bufferCV;

            std::atomic<bool> seekPending{ false };

            VideoSystem* owner;
        };

        struct SaveTask {
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
            PlaybackMode mode = PlaybackMode::Cached;
            std::future<LoadResult> future;
        };

        struct PendingRestore {
            double keepTime = 0.0;
            bool wasPlaying = false;
        };

        std::unordered_map<EntityID, std::unique_ptr<Track>> m_tracks;
        mutable std::mutex m_tracksMutex;

        std::vector<LoadingTask> m_pendingLoads;
        mutable std::recursive_mutex m_loadMutex;

        std::unordered_map<EntityID, PendingRestore> m_pendingRestores;
        mutable std::mutex m_restoreMutex;

        std::unordered_map<EntityID, std::future<bool>> m_saveFutures;
        std::unordered_map<EntityID, std::string>      m_savePaths;
        mutable std::mutex m_saveMutex;

        std::vector<std::pair<void*, VideoCallback>>        m_addedCallbacks;
        std::vector<std::pair<void*, VideoCallback>>        m_removedCallbacks;
        std::vector<std::pair<void*, VideoTextureCallback>> m_textureCallbacks;
        std::vector<std::pair<void*, SaveCallback>>         m_saveCallbacks;
        std::vector<std::pair<void*, LoadCallback>>         m_loadCallbacks;
        std::vector<std::pair<void*, EndCallback>>          m_endCallbacks;

        AudioSystem* m_audio = nullptr;
        std::atomic<bool> m_destroying{ false };

        static LoadResult LoadVideoInBackground(const std::string& filePath, EntityID entity);
        static bool SaveVideoInBackground(const SaveTask& task);

        void LoadVideoAsync(EntityID entity, const std::string& filePath, PlaybackMode mode);
        void ProcessCompletedLoads();
        void ProcessCompletedSaves();
        void ApplyLoadedVideo(LoadResult&& result);

        void WorkerLoop(Track& track);

        void NotifyVideoAdded(EntityID entity);
        void NotifyVideoRemoved(EntityID entity);
        void NotifyLoadComplete(EntityID entity, bool success);
        void NotifySaveComplete(EntityID entity, bool success, const std::string& path);
        void NotifyEnd(EntityID entity);
    };

} // namespace ECS