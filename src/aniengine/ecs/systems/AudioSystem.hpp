#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AudioComponent.hpp"
#include "PlaybackStateComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include <portaudio.h>
#include <memory>
#include <functional>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <future>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace ECS {

    class AudioSystem : public BaseSystem {
    public:
        using AudioCallback = std::function<void(EntityID)>;
        using AudioDataCallback = std::function<void(EntityID, const float*, size_t, int, int)>;
        using EndCallback = std::function<void(EntityID)>;

        struct LoadResult {
            bool success = false;
            EntityID entityID = 0;
            std::string filePath;
            std::string fileName;
            double duration = 0.0;
            int channels = 0;
            int sampleRate = 0;
            int64_t totalSamples = 0;
            std::vector<float> pcmData;
            bool hasExif = false;
            bool hasLSB = false;
            bool hasAniStudio = false;
            AVFormatContext* fmtCtx = nullptr;
            AVCodecContext* codecCtx = nullptr;
            SwrContext* swrCtx = nullptr;
            int audioStreamIndex = -1;

            LoadResult() = default;
            ~LoadResult() {
                if (fmtCtx) avformat_close_input(&fmtCtx);
                if (codecCtx) avcodec_free_context(&codecCtx);
                if (swrCtx) swr_free(&swrCtx);
            }
            LoadResult(const LoadResult&) = delete;
            LoadResult& operator=(const LoadResult&) = delete;
            LoadResult(LoadResult&& o) noexcept { *this = std::move(o); }
            LoadResult& operator=(LoadResult&& o) noexcept {
                if (this != &o) {
                    if (fmtCtx) avformat_close_input(&fmtCtx);
                    if (codecCtx) avcodec_free_context(&codecCtx);
                    if (swrCtx) swr_free(&swrCtx);
                    success = o.success; entityID = o.entityID;
                    filePath = std::move(o.filePath); fileName = std::move(o.fileName);
                    duration = o.duration;
                    channels = o.channels; sampleRate = o.sampleRate;
                    totalSamples = o.totalSamples;
                    pcmData = std::move(o.pcmData);
                    hasExif = o.hasExif; hasLSB = o.hasLSB; hasAniStudio = o.hasAniStudio;
                    fmtCtx = o.fmtCtx; codecCtx = o.codecCtx; swrCtx = o.swrCtx;
                    audioStreamIndex = o.audioStreamIndex;
                    o.fmtCtx = nullptr; o.codecCtx = nullptr; o.swrCtx = nullptr;
                    o.audioStreamIndex = -1;
                }
                return *this;
            }
        };

        AudioSystem(EntityManager& entityMgr);
        ~AudioSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;
        void OnEntityDestroyed(EntityID entity) override;

        void LoadAudio(EntityID entity, const std::string& filePath,
            PlaybackMode mode = PlaybackMode::Cached);
        void SetAudio(EntityID entity, const std::string& filePath,
            PlaybackMode mode = PlaybackMode::Cached) {
            LoadAudio(entity, filePath, mode);
        }

        void AddLoadedAudio(EntityID entity);
        void AddSilentTrack(EntityID entity, double duration);
        void RemoveAudio(EntityID entity);
        void ClearCache(EntityID entity);
        void SetMode(EntityID entity, PlaybackMode mode,
            double keepTime, bool wasPlaying);

        bool HasTrack(EntityID entity) const;
        bool IsSilentTrack(EntityID entity) const;

        static LoadResult DecodeAudioFile(const std::string& filePath, EntityID entity,
            int targetSampleRate, int targetChannels);
        static LoadResult OpenAudioStream(const std::string& filePath, EntityID entity,
            int targetSampleRate, int targetChannels);
        static LoadResult ExtractAudioFromVideoFile(const std::string& filePath, EntityID entity);

        void Play(EntityID entity, bool loop = false);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetVolume(EntityID entity, float volume);
        void SetSpeed(EntityID entity, float speed);

        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;
        bool IsLoading(EntityID entity) const;

        double GetCurrentPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;

        std::vector<EntityID> GetAllAudioEntities() const;

        void RegisterAudioAddedCallback(void* owner, const AudioCallback& cb);
        void RegisterAudioRemovedCallback(void* owner, const AudioCallback& cb);
        void RegisterAudioDataCallback(void* owner, const AudioDataCallback& cb);
        void RegisterEndCallback(void* owner, const EndCallback& cb);
        void UnregisterCallbacksForOwner(void* owner);

        void PlayTestTone();

    private:
        struct StreamState;

        struct Track {
            Track(AudioSystem* owner, EntityID e);
            ~Track();

            void StartProducerThread();

            EntityID entity = 0;
            PlaybackMode mode = PlaybackMode::Cached;

            std::vector<float> pcmData;
            size_t totalSamples = 0;
            int channels = 0;
            int sampleRate = 0;
            double duration = 0.0;

            size_t readPosition = 0;
            bool silent = false;
            bool paused = true;
            bool stopped = true;
            bool endReached = false;
            bool loop = false;

            float volume = 1.0f;
            float speed = 1.0f;
            double streamTime = 0.0;

            std::vector<float> ring;
            size_t ringWrite = 0;
            size_t ringRead = 0;
            size_t ringCapacitySamples = 0;
            size_t ringAvailable = 0;
            bool eofReached = false;

            std::thread producer;
            std::atomic<bool> running{ true };
            std::atomic<bool> seekRequested{ false };
            std::atomic<double> seekTarget{ 0.0 };
            std::mutex ringMutex;
            std::condition_variable ringCV;

            AudioSystem* owner;
        };

        struct StreamState {
            PaStream* stream = nullptr;
            std::unordered_map<EntityID, std::unique_ptr<Track>> tracks;
            mutable std::mutex mutex;
            std::atomic<bool> running{ false };
            bool streamOpen = false;
        };

        struct PendingRestore {
            double keepTime = 0.0;
            bool wasPlaying = false;
        };

        std::unique_ptr<StreamState> m_state;
        std::atomic<bool> m_destroying{ false };

        struct LoadingTask {
            EntityID entityID;
            std::string filePath;
            PlaybackMode mode = PlaybackMode::Cached;
            std::future<LoadResult> future;
        };
        std::vector<LoadingTask> m_pendingLoads;
        mutable std::mutex m_loadMutex;

        std::unordered_map<EntityID, PendingRestore> m_pendingRestores;
        mutable std::mutex m_restoreMutex;

        std::vector<std::pair<void*, AudioCallback>>     m_addedCallbacks;
        std::vector<std::pair<void*, AudioCallback>>     m_removedCallbacks;
        std::vector<std::pair<void*, AudioDataCallback>> m_dataCallbacks;
        std::vector<std::pair<void*, EndCallback>>       m_endCallbacks;

        bool OpenStream();
        void CloseStream();
        void ProcessCompletedLoads();
        void LoadAudioAsync(EntityID entity, const std::string& filePath, PlaybackMode mode);

        static int PaCallback(const void* input, void* output,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        void NotifyAudioAdded(EntityID entity);
        void NotifyAudioRemoved(EntityID entity);
        void NotifyAudioData(EntityID entity, const float* data, size_t size,
            int channels, int sampleRate);
        void NotifyEnd(EntityID entity);
    };

} // namespace ECS