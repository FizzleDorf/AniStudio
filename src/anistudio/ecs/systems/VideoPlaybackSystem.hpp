#pragma once
#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"
#include "AudioPlaybackSystem.hpp"
#include "ThreadPoolSystem.hpp"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <deque>
#include <atomic>
#include <functional>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace ECS {

    class VideoPlaybackSystem : public BaseSystem {
    public:
        using VideoPlaybackCallback = std::function<void(EntityID, const unsigned char*, int, int)>;
        using PlaybackEndCallback = std::function<void(EntityID)>;

        VideoPlaybackSystem(EntityManager& entityMgr);
        ~VideoPlaybackSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterVideoPlaybackCallback(const VideoPlaybackCallback& cb);
        void RegisterPlaybackEndCallback(const PlaybackEndCallback& cb);

        void Play(EntityID entity, bool loop = false);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetSpeed(EntityID entity, float speed);
        void SetVolume(EntityID entity, float volume);

        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;
        double GetCurrentPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;

    private:
        struct FrameIndexEntry {
            int64_t frameIndex;
            int64_t packetPos;
            int64_t keyframePos;
            int64_t pts;
            bool isKeyframe;
        };

        struct RingFrame {
            std::vector<uint8_t> data;
            int width = 0;
            int height = 0;
            double pts = 0.0;
            long long frameIndex = 0;
        };

        enum class WorkerCmdType {
            Seek,
            Pause,
            Resume,
            Shutdown,
            Stop,
            BuildIndex
        };

        struct WorkerCmd {
            WorkerCmdType type;
            double seekTime = 0.0;
            bool pauseAfter = false;
        };

        struct VideoTrack {
            VideoTrack(VideoPlaybackSystem* owner, EntityManager& mgr, EntityID entity,
                bool hasAudio, double duration, double fps);
            ~VideoTrack();

            void StartThread();
            void PushCommand(WorkerCmd cmd);
            void RequestSeek(double time, bool pauseAfter = false);
            void RequestPause();
            void RequestResume();
            void RequestStop();
            void RequestShutdown();
            void RequestBuildIndex();
            void ClearBuffer();
            bool TryPopDisplayFrame(double upToTime, RingFrame& out);
            bool DecodeOneFrame(RingFrame& out);
            bool DecodeFrameAt(int64_t targetPts, RingFrame& out);
            void BuildFrameIndex();

            EntityID entity;
            bool hasAudio = false;
            double duration = 0.0;
            double fps = 30.0;
            bool paused = false;
            bool reachedEnd = false;
            bool loop = false;
            bool stopped = false;
            bool seeking = false;
            bool seekPending = false;
            bool firstFrameAfterSeek = false;
            double pendingSeekTime = 0.0;
            double pendingSeekTimeStore = -1.0;
            double lastDisplayedPts = 0.0;
            bool wasPlayingBeforeSeek = false;
            bool indexReady = false;
            std::vector<FrameIndexEntry> frameIndex;
            AVRational streamTimeBase;
            int64_t lastKeyframePts = 0;

            class Clock {
            public:
                void Reset(double startTime = 0.0);
                void Play();
                void Pause();
                void Seek(double time);
                void SetSpeed(float speed);
                double Now() const;

            private:
                double m_currentTime = 0.0;
                double m_speed = 1.0f;
                bool m_paused = true;
                mutable std::chrono::steady_clock::time_point m_lastUpdate;
            };
            Clock clock;

        private:
            static constexpr size_t kBufferCapacity = 8;

            void WorkerLoop();
            void PerformSeek(double time);

            VideoPlaybackSystem* m_owner;
            EntityManager& m_mgr;

            std::thread m_thread;
            std::atomic<bool> m_running{ true };
            std::atomic<bool> m_workerPaused{ false };
            std::atomic<bool> m_indexing{ false };

            std::deque<WorkerCmd> m_commands;
            mutable std::mutex m_cmdMutex;
            std::condition_variable m_cmdCV;

            std::deque<RingFrame> m_buffer;
            mutable std::mutex m_bufferMutex;
            std::condition_variable m_bufferCV;
        };

        std::unordered_map<EntityID, std::unique_ptr<VideoTrack>> m_tracks;
        mutable std::mutex m_tracksMutex;

        std::vector<VideoPlaybackCallback> m_callbacks;
        std::vector<PlaybackEndCallback> m_endCallbacks;

        AudioPlaybackSystem* m_audioPlayback = nullptr;
        std::atomic<bool> m_destroying{ false };

        void NotifyPlaybackEnd(EntityID entity);
        void RemoveTrack(EntityID entity);
        void HandleEndOfStream(EntityID entity, VideoTrack& track);
    };

}