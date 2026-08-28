#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AudioComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <chrono>
#include <thread>
#include <atomic>
#include <portaudio.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}

namespace ECS {

    class AudioSystem : public BaseSystem {
    public:
        using AudioCallback = std::function<void(EntityID)>;
        using AudioDataCallback = std::function<void(EntityID, const float*, size_t, int, int)>;

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
        };

        struct LoadingTask {
            EntityID entityID;
            std::string filePath;
            std::future<LoadResult> future;

            LoadingTask() = default;
            LoadingTask(LoadingTask&& other) noexcept
                : entityID(other.entityID)
                , filePath(std::move(other.filePath))
                , future(std::move(other.future)) {
            }
            LoadingTask& operator=(LoadingTask&& other) noexcept {
                if (this != &other) {
                    entityID = other.entityID;
                    filePath = std::move(other.filePath);
                    future = std::move(other.future);
                }
                return *this;
            }
            LoadingTask(const LoadingTask&) = delete;
            LoadingTask& operator=(const LoadingTask&) = delete;
        };

        AudioSystem(EntityManager& entityMgr);
        ~AudioSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterAudioAddedCallback(const AudioCallback& callback);
        void RegisterAudioRemovedCallback(const AudioCallback& callback);
        void RegisterAudioDataCallback(const AudioDataCallback& callback);

        void SetAudio(EntityID entity, const std::string& filePath);
        void RemoveAudio(EntityID entity);
        std::vector<EntityID> GetAllAudioEntities() const;

        void Play(EntityID entity, bool loop = false);
        void Stop(EntityID entity);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void SetVolume(EntityID entity, float volume);
        void Seek(EntityID entity, double position);
        double GetCurrentPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;
        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;

        const float* GetAudioData(EntityID entity, size_t& outSize, int& outChannels) const;

        std::vector<std::string> GetAvailableDevices() const;
        bool SetOutputDevice(int deviceIndex);

        void PlayTestTone();

    private:
        std::vector<AudioCallback> audioAddedCallbacks;
        std::vector<AudioCallback> audioRemovedCallbacks;
        std::vector<AudioDataCallback> audioDataCallbacks;
        std::vector<LoadingTask> pendingLoads;
        mutable std::mutex loadMutex;
        mutable std::mutex playbackMutex;

        void LoadAudioAsync(EntityID entity, const std::string& filePath);
        void ProcessCompletedLoads();
        void NotifyAudioAdded(EntityID entity);
        void NotifyAudioRemoved(EntityID entity);
        void NotifyAudioData(EntityID entity, const float* data, size_t size, int channels, int sampleRate);

        static LoadResult DecodeAudioFile(const std::string& filePath, EntityID entity, int targetSampleRate, int targetChannels);
        void UpdatePlayback(AudioComponent& audioComp, float deltaT);
        void ReopenStream();

        // Blocking I/O thread
        void AudioPlaybackThread();
        std::thread m_playbackThread;
        std::atomic<bool> m_playbackThreadRunning{ false };
        std::condition_variable m_playbackCV;
        std::mutex m_playbackCV_mutex;

        PaStream* m_paStream = nullptr;
        std::atomic<bool> m_paInitialized{ false };
        std::atomic<int> m_paDeviceIndex{ -1 };

        int m_streamSampleRate = 0;
        int m_streamChannels = 0;
        int m_streamFramesPerBuffer = 1024;
        bool m_streamInitialized = false;
        bool m_streamNeedsReopen = false;
        int m_deviceSampleRate = 0;
        int m_actualStreamSampleRate = 0;

        EntityID m_playingEntity = 0;
        float m_playbackVolume = 1.0f;
        std::atomic<bool> m_playbackActive{ false };
        std::atomic<bool> m_playbackPaused{ false };
    };

} // namespace ECS