#include "AudioPlaybackSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace ECS {

    AudioPlaybackSystem::AudioPlaybackSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr)
        , m_streamState(std::make_unique<AudioStreamState>()) {
        sysName = "AudioPlaybackSystem";
        AddComponentSignature<AudioComponent>();

        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to initialize PortAudio: "
                << Pa_GetErrorText(err) << std::endl;
        }
    }

    AudioPlaybackSystem::~AudioPlaybackSystem() {
        Destroy();
        Pa_Terminate();
    }

    void AudioPlaybackSystem::Start() {
        OpenStream();
    }

    void AudioPlaybackSystem::Update(float deltaT) {
        // Track state updates are handled in the audio callback
    }

    void AudioPlaybackSystem::Destroy() {
        m_destroying = true;
        CloseStream();

        std::lock_guard<std::mutex> lock(m_trackMutex);
        m_tracks.clear();
    }

    bool AudioPlaybackSystem::OpenStream() {
        if (m_streamState->streamOpen) return true;

        PaStreamParameters outputParams;
        outputParams.device = Pa_GetDefaultOutputDevice();
        if (outputParams.device == paNoDevice) {
            std::cerr << "[AudioPlaybackSystem] No audio output device available" << std::endl;
            return false;
        }

        outputParams.channelCount = 2;
        outputParams.sampleFormat = paFloat32;
        outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
        outputParams.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(
            &m_streamState->stream,
            nullptr,
            &outputParams,
            44100,
            256,
            paNoFlag,
            &AudioPlaybackSystem::PaCallback,
            m_streamState.get()
        );

        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to open stream: "
                << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        m_streamState->streamOpen = true;
        m_streamState->running = true;

        err = Pa_StartStream(m_streamState->stream);
        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to start stream: "
                << Pa_GetErrorText(err) << std::endl;
            CloseStream();
            return false;
        }

        return true;
    }

    void AudioPlaybackSystem::CloseStream() {
        m_streamState->running = false;

        if (m_streamState->stream) {
            Pa_StopStream(m_streamState->stream);
            Pa_CloseStream(m_streamState->stream);
            m_streamState->stream = nullptr;
            m_streamState->streamOpen = false;
        }
    }

    int AudioPlaybackSystem::PaCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData) {

        AudioStreamState* state = static_cast<AudioStreamState*>(userData);
        if (!state || !state->running.load()) {
            return paComplete;
        }

        float* out = static_cast<float*>(outputBuffer);
        size_t samples = framesPerBuffer * 2;
        std::fill(out, out + samples, 0.0f);

        std::lock_guard<std::mutex> lock(state->mutex);

        for (auto& pair : state->tracks) {
            AudioTrackState& track = pair.second;

            if (track.stopped || track.paused || !track.pcmData) {
                continue;
            }

            size_t availableSamples = track.totalSamples - track.readPosition;
            if (availableSamples == 0) {
                if (track.loop) {
                    track.readPosition = 0;
                    availableSamples = track.totalSamples;
                }
                else {
                    track.endReached = true;
                    continue;
                }
            }

            size_t framesToRead = std::min<size_t>(framesPerBuffer, availableSamples / track.channels);
            size_t samplesToRead = framesToRead * track.channels;
            size_t readPos = track.readPosition;

            const float* src = track.pcmData + readPos;
            float* dst = out;

            float vol = track.volume;
            int channels = track.channels;

            if (channels == 2) {
                for (size_t i = 0; i < samplesToRead; i += 2) {
                    dst[i] += src[i] * vol;
                    dst[i + 1] += src[i + 1] * vol;
                }
            }
            else {
                for (size_t i = 0; i < samplesToRead; ++i) {
                    float sample = src[i] * vol;
                    dst[i * 2] += sample;
                    dst[i * 2 + 1] += sample;
                }
            }

            track.readPosition += samplesToRead;
            track.streamTime = static_cast<double>(track.readPosition) /
                (track.sampleRate * track.channels);
            track.lastPaTime = timeInfo->outputBufferDacTime;
        }

        return paContinue;
    }

    void AudioPlaybackSystem::RegisterPlaybackCallback(const AudioPlaybackCallback& cb) {
        m_callbacks.push_back(cb);
    }

    void AudioPlaybackSystem::RegisterEndCallback(const AudioEndCallback& cb) {
        m_endCallbacks.push_back(cb);
    }

    void AudioPlaybackSystem::Play(EntityID entity, bool loop) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        if (audioComp.pcmData.empty()) {
            return;
        }

        if (!m_streamState->streamOpen) {
            if (!OpenStream()) {
                return;
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_trackMutex);

            auto it = m_tracks.find(entity);
            if (it != m_tracks.end()) {
                if (!it->second.paused) {
                    return;
                }
                it->second.paused = false;
                it->second.stopped = false;
                it->second.endReached = false;
                it->second.loop = loop;
                it->second.streamTime = it->second.readPosition / (it->second.sampleRate * it->second.channels);
            }
            else {
                AudioTrackState track;
                track.entity = entity;
                track.pcmData = audioComp.pcmData.data();
                track.totalSamples = audioComp.pcmData.size();
                track.channels = audioComp.channels;
                track.sampleRate = audioComp.sampleRate;
                track.duration = audioComp.duration;
                track.readPosition = 0;
                track.paused = false;
                track.stopped = false;
                track.loop = loop;
                track.endReached = false;
                track.volume = audioComp.volume;
                track.speed = 1.0f;
                track.streamTime = 0.0;
                track.lastPaTime = 0.0;

                m_tracks[entity] = std::move(track);
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_streamState->mutex);
            auto it = m_tracks.find(entity);
            if (it != m_tracks.end()) {
                m_streamState->tracks[entity] = it->second;
            }
        }
    }

    void AudioPlaybackSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end()) {
            it->second.paused = true;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.paused = true;
                }
            }
        }
    }

    void AudioPlaybackSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end() && it->second.paused) {
            it->second.paused = false;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.paused = false;
                }
            }
        }
    }

    void AudioPlaybackSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end()) {
            it->second.stopped = true;
            it->second.paused = true;
            it->second.readPosition = 0;
            it->second.streamTime = 0.0;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.stopped = true;
                    streamIt->second.paused = true;
                    streamIt->second.readPosition = 0;
                    streamIt->second.streamTime = 0.0;
                }
            }
        }
    }

    void AudioPlaybackSystem::Seek(EntityID entity, double time) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        if (audioComp.pcmData.empty()) {
            return;
        }

        double duration = audioComp.duration;
        time = std::clamp(time, 0.0, duration - 0.001);

        size_t position = static_cast<size_t>(time * audioComp.sampleRate * audioComp.channels);
        position = (position / audioComp.channels) * audioComp.channels;
        position = std::min(position, audioComp.pcmData.size() - audioComp.channels);

        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end()) {
            it->second.readPosition = position;
            it->second.streamTime = time;
            it->second.endReached = false;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.readPosition = position;
                    streamIt->second.streamTime = time;
                    streamIt->second.endReached = false;
                }
            }
        }
    }

    void AudioPlaybackSystem::SetVolume(EntityID entity, float volume) {
        volume = std::clamp(volume, 0.0f, 1.0f);
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end()) {
            it->second.volume = volume;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.volume = volume;
                }
            }
        }
    }

    void AudioPlaybackSystem::SetPlaybackSpeed(EntityID entity, float speed) {
        speed = std::clamp(speed, 0.1f, 4.0f);
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        if (it != m_tracks.end()) {
            it->second.speed = speed;
            {
                std::lock_guard<std::mutex> streamLock(m_streamState->mutex);
                auto streamIt = m_streamState->tracks.find(entity);
                if (streamIt != m_streamState->tracks.end()) {
                    streamIt->second.speed = speed;
                }
            }
        }
    }

    bool AudioPlaybackSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && !it->second.paused && !it->second.stopped;
    }

    bool AudioPlaybackSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && it->second.paused && !it->second.stopped;
    }

    double AudioPlaybackSystem::GetCurrentPosition(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_streamState->mutex);
        auto it = m_streamState->tracks.find(entity);
        if (it != m_streamState->tracks.end()) {
            return it->second.streamTime;
        }
        return 0.0;
    }

    double AudioPlaybackSystem::GetDuration(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return 0.0;
        }
        return mgr.GetComponent<AudioComponent>(entity).duration;
    }

    void AudioPlaybackSystem::PlayTestTone() {
        std::cout << "[AudioPlaybackSystem] Playing test tone..." << std::endl;

        if (!m_streamState->streamOpen) {
            if (!OpenStream()) {
                return;
            }
        }

        const int sampleRate = 44100;
        const int channels = 2;
        const int duration = 2;
        const float frequency = 440.0f;
        const float amplitude = 0.5f;

        std::vector<float> toneData;
        size_t totalSamples = sampleRate * duration * channels;
        toneData.resize(totalSamples);

        for (size_t i = 0; i < totalSamples; i += channels) {
            float sample = amplitude * sinf(2.0f * 3.14159f * frequency * (i / channels) / sampleRate);
            toneData[i] = sample;
            toneData[i + 1] = sample;
        }

        EntityID entity = 0;
        for (auto e : mgr.GetAllEntities()) {
            if (mgr.HasComponent<AudioComponent>(e)) {
                entity = e;
                break;
            }
        }

        if (entity == 0) {
            entity = mgr.AddNewEntity();
            mgr.AddComponent<AudioComponent>(entity);
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            audioComp.pcmData = std::move(toneData);
            audioComp.channels = channels;
            audioComp.sampleRate = sampleRate;
            audioComp.duration = duration;
            audioComp.fileName = "Test Tone";
        }
        else {
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            audioComp.pcmData = std::move(toneData);
            audioComp.channels = channels;
            audioComp.sampleRate = sampleRate;
            audioComp.duration = duration;
        }

        Play(entity, false);
    }

    void AudioPlaybackSystem::NotifyPlaybackEnd(EntityID entity) {
        for (const auto& cb : m_endCallbacks) {
            try {
                cb(entity);
            }
            catch (...) {}
        }
    }

}