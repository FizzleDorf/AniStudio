#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AudioComponent.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <portaudio.h>

namespace ECS {

    class AudioPlaybackSystem : public BaseSystem {
    public:
        using AudioPlaybackCallback = std::function<void(EntityID, const float*, size_t, int, int)>;

        AudioPlaybackSystem(EntityManager& entityMgr);
        ~AudioPlaybackSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterPlaybackCallback(const AudioPlaybackCallback& callback);

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

        std::vector<std::string> GetAvailableDevices() const;
        bool SetOutputDevice(int deviceIndex);

        void PlayTestTone();

    private:
        void ReopenStream();
        void AudioPlaybackThread();

        std::vector<AudioPlaybackCallback> m_callbacks;

        PaStream* m_paStream = nullptr;
        std::atomic<bool> m_paInitialized{ false };
        std::atomic<int> m_paDeviceIndex{ -1 };

        int m_streamSampleRate = 44100;
        int m_streamChannels = 2;
        int m_streamFramesPerBuffer = 1024;
        bool m_streamInitialized = false;
        int m_deviceSampleRate = 0;
        int m_actualStreamSampleRate = 0;

        std::thread m_playbackThread;
        std::atomic<bool> m_playbackThreadRunning{ false };
        std::condition_variable m_playbackCV;
        std::mutex m_playbackCV_mutex;

        EntityID m_playingEntity = 0;
        float m_playbackVolume = 1.0f;
        std::atomic<bool> m_playbackActive{ false };
        std::atomic<bool> m_playbackPaused{ false };

        mutable std::mutex m_playbackMutex;
    };

} // namespace ECS