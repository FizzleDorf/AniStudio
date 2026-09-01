#pragma once
#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "AudioComponent.hpp"
#include "ThreadPoolSystem.hpp"
#include <portaudio.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory>

namespace ECS {

    class AudioPlaybackSystem : public BaseSystem {
    public:
        using AudioPlaybackCallback = std::function<void(EntityID, const float*, size_t, int)>;
        using AudioEndCallback = std::function<void(EntityID)>;

        AudioPlaybackSystem(EntityManager& entityMgr);
        ~AudioPlaybackSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void RegisterPlaybackCallback(const AudioPlaybackCallback& cb);
        void RegisterEndCallback(const AudioEndCallback& cb);

        void Play(EntityID entity, bool loop = false);
        void Pause(EntityID entity);
        void Resume(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetVolume(EntityID entity, float volume);
        void SetPlaybackSpeed(EntityID entity, float speed);

        bool IsPlaying(EntityID entity) const;
        bool IsPaused(EntityID entity) const;
        double GetCurrentPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;

        void PlayTestTone();

    private:
        struct AudioTrackState {
            EntityID entity = 0;
            const float* pcmData = nullptr;
            size_t totalSamples = 0;
            int channels = 0;
            int sampleRate = 0;
            double duration = 0.0;
            size_t readPosition = 0;
            bool paused = true;
            bool loop = false;
            bool stopped = true;
            bool endReached = false;
            float volume = 1.0f;
            float speed = 1.0f;
            double lastPaTime = 0.0;
            double streamTime = 0.0;
        };

        struct AudioStreamState {
            PaStream* stream = nullptr;
            std::unordered_map<EntityID, AudioTrackState> tracks;
            mutable std::mutex mutex;
            std::atomic<bool> running{ false };
            bool streamOpen = false;
        };

        std::unordered_map<EntityID, AudioTrackState> m_tracks;
        mutable std::mutex m_trackMutex;
        std::unique_ptr<AudioStreamState> m_streamState;

        std::vector<AudioPlaybackCallback> m_callbacks;
        std::vector<AudioEndCallback> m_endCallbacks;

        std::atomic<bool> m_destroying{ false };

        static int PaCallback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        void ProcessTracks(float* outputBuffer, unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo);
        void NotifyPlaybackEnd(EntityID entity);
        bool OpenStream();
        void CloseStream();
    };

}