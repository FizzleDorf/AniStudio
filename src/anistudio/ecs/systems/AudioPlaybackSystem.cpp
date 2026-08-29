#include "AudioPlaybackSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

namespace ECS {

    AudioPlaybackSystem::AudioPlaybackSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "AudioPlaybackSystem";
        AddComponentSignature<AudioComponent>();

        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            m_paInitialized = true;
            std::cout << "[AudioPlaybackSystem] PortAudio initialized" << std::endl;

            int numDevices = Pa_GetDeviceCount();
            std::cout << "[AudioPlaybackSystem] Found " << numDevices << " audio devices" << std::endl;
            for (int i = 0; i < numDevices; i++) {
                const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
                if (info) {
                    std::cout << "  Device " << i << ": " << info->name
                        << " (max out channels: " << info->maxOutputChannels << ")" << std::endl;
                }
            }

            m_paDeviceIndex = Pa_GetDefaultOutputDevice();
            std::cout << "[AudioPlaybackSystem] Default output device: " << m_paDeviceIndex << std::endl;
            if (m_paDeviceIndex >= 0) {
                const PaDeviceInfo* info = Pa_GetDeviceInfo(m_paDeviceIndex);
                if (info) {
                    m_deviceSampleRate = static_cast<int>(info->defaultSampleRate);
                    std::cout << "[AudioPlaybackSystem] Device default sample rate: " << m_deviceSampleRate << " Hz" << std::endl;
                }
            }
        }

        // Initialize libsamplerate
        int srcError = 0;
        m_srcState = src_new(SRC_SINC_FASTEST, 2, &srcError);
        if (m_srcState) {
            std::cout << "[AudioPlaybackSystem] libsamplerate initialized" << std::endl;
        }
        else {
            std::cerr << "[AudioPlaybackSystem] Failed to initialize libsamplerate: " << src_strerror(srcError) << std::endl;
        }

        std::cout << "[AudioPlaybackSystem] Initialized" << std::endl;
    }

    AudioPlaybackSystem::~AudioPlaybackSystem() {
        std::cout << "[AudioPlaybackSystem] Destructor - cleaning up" << std::endl;

        {
            std::lock_guard<std::mutex> lock(m_playbackMutex);
            m_playbackActive = false;
            m_playingEntity = 0;
        }
        m_playbackCV.notify_one();
        if (m_playbackThread.joinable()) {
            m_playbackThread.join();
        }

        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }

        if (m_paInitialized) {
            Pa_Terminate();
            m_paInitialized = false;
        }

        if (m_srcState) {
            src_delete(m_srcState);
            m_srcState = nullptr;
        }
    }

    void AudioPlaybackSystem::ReopenStream() {
        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }

        if (!m_paInitialized || m_paDeviceIndex < 0) {
            return;
        }

        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_paDeviceIndex);
        if (deviceInfo && m_streamChannels > deviceInfo->maxOutputChannels) {
            m_streamChannels = deviceInfo->maxOutputChannels;
        }

        std::cout << "[AudioPlaybackSystem] Opening stream: " << m_streamSampleRate << "Hz, "
            << m_streamChannels << "ch" << std::endl;

        PaStreamParameters outputParameters;
        outputParameters.device = m_paDeviceIndex;
        outputParameters.channelCount = m_streamChannels;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(m_paDeviceIndex)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&m_paStream,
            nullptr,
            &outputParameters,
            m_streamSampleRate,
            m_streamFramesPerBuffer,
            paClipOff,
            nullptr,
            nullptr);

        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }

        const PaStreamInfo* streamInfo = Pa_GetStreamInfo(m_paStream);
        if (streamInfo) {
            m_actualStreamSampleRate = static_cast<int>(streamInfo->sampleRate);
            std::cout << "[AudioPlaybackSystem] Actual stream sample rate: " << m_actualStreamSampleRate << " Hz" << std::endl;
            if (m_actualStreamSampleRate != m_streamSampleRate) {
                std::cout << "[AudioPlaybackSystem] WARNING: Stream opened at different sample rate. Adjusting to match." << std::endl;
                m_streamSampleRate = m_actualStreamSampleRate;
            }
        }

        err = Pa_StartStream(m_paStream);
        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            m_streamInitialized = true;
            std::cout << "[AudioPlaybackSystem] PortAudio stream started at " << m_streamSampleRate << " Hz" << std::endl;
        }
    }

    void AudioPlaybackSystem::Start() {
        std::cout << "[AudioPlaybackSystem] Started" << std::endl;

        if (!m_playbackThreadRunning) {
            m_playbackThreadRunning = true;
            m_playbackThread = std::thread(&AudioPlaybackSystem::AudioPlaybackThread, this);
        }

        ReopenStream();
    }

    void AudioPlaybackSystem::Update(float deltaT) {
    }

    void AudioPlaybackSystem::Destroy() {
        std::cout << "[AudioPlaybackSystem] Destroying" << std::endl;

        {
            std::lock_guard<std::mutex> lock(m_playbackMutex);
            m_playbackActive = false;
            m_playingEntity = 0;
        }
        m_playbackCV.notify_one();
        if (m_playbackThread.joinable()) {
            m_playbackThread.join();
        }

        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }
    }

    void AudioPlaybackSystem::RegisterPlaybackCallback(const AudioPlaybackCallback& callback) {
        m_callbacks.push_back(callback);
    }

    void AudioPlaybackSystem::SetPlaybackSpeed(EntityID entity, float speed) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);
        if (m_playingEntity != entity) {
            return; // Not playing this entity
        }
        m_playbackSpeed = std::clamp(speed, 0.1f, 4.0f);
        m_playbackSpeedRatio = 1.0 / m_playbackSpeed;
        if (m_srcState) {
            src_reset(m_srcState);
        }
        std::cout << "[AudioPlaybackSystem] Playback speed set to " << m_playbackSpeed << "x (ratio: " << m_playbackSpeedRatio << ")" << std::endl;
    }

    void AudioPlaybackSystem::Play(EntityID entity, bool loop) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);

        if (audioComp.pcmData.empty()) {
            std::cerr << "[AudioPlaybackSystem] No audio data loaded for entity " << entity << std::endl;
            return;
        }

        audioComp.looping = loop;
        audioComp.isPlaying = true;
        audioComp.isPaused = false;
        m_playingEntity = entity;
        m_playbackActive = true;
        m_playbackPaused = false;

        if (audioComp.currentSampleIndex >= audioComp.pcmData.size() / audioComp.channels) {
            audioComp.currentSampleIndex = 0;
            audioComp.currentTime = 0.0;
        }

        // Reset resampler state on new play
        if (m_srcState) {
            src_reset(m_srcState);
        }

        m_playbackCV.notify_one();

        std::cout << "[AudioPlaybackSystem] Playing audio: " << audioComp.fileName << std::endl;
    }

    void AudioPlaybackSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (m_playingEntity != entity) {
            return;
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            audioComp.isPlaying = false;
            audioComp.isPaused = false;
            audioComp.currentSampleIndex = 0;
            audioComp.currentTime = 0.0;
        }

        m_playbackActive = false;
        m_playingEntity = 0;
        m_playbackPaused = false;
        m_playbackCV.notify_one();
    }

    void AudioPlaybackSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (m_playingEntity != entity) {
            return;
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            if (audioComp.isPlaying) {
                audioComp.isPaused = true;
                m_playbackPaused = true;
            }
        }
    }

    void AudioPlaybackSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (m_playingEntity != entity) {
            return;
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            if (audioComp.isPlaying && audioComp.isPaused) {
                audioComp.isPaused = false;
                m_playbackPaused = false;
                if (m_srcState) {
                    src_reset(m_srcState);
                }
                m_playbackCV.notify_one();
            }
        }
    }

    void AudioPlaybackSystem::SetVolume(EntityID entity, float volume) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.volume = std::clamp(volume, 0.0f, 1.0f);
        if (m_playingEntity == entity) {
            m_playbackVolume = audioComp.volume;
        }
    }

    void AudioPlaybackSystem::Seek(EntityID entity, double position) {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (m_playingEntity != entity) {
            return;
        }

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        if (audioComp.pcmData.empty()) {
            return;
        }

        position = std::clamp(position, 0.0, audioComp.duration);
        audioComp.currentTime = position;
        audioComp.currentSampleIndex = static_cast<size_t>(position * audioComp.sampleRate) * audioComp.channels;

        if (audioComp.currentSampleIndex >= audioComp.pcmData.size() / audioComp.channels) {
            audioComp.currentSampleIndex = 0;
            audioComp.currentTime = 0.0;
        }

        if (m_srcState) {
            src_reset(m_srcState);
        }
    }

    double AudioPlaybackSystem::GetCurrentPosition(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return 0.0;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.currentTime;
    }

    double AudioPlaybackSystem::GetDuration(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return 0.0;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.duration;
    }

    bool AudioPlaybackSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return false;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.isPlaying && !audioComp.isPaused;
    }

    bool AudioPlaybackSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return false;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.isPlaying && audioComp.isPaused;
    }

    std::vector<std::string> AudioPlaybackSystem::GetAvailableDevices() const {
        std::vector<std::string> devices;

        if (!m_paInitialized) {
            devices.push_back("Default");
            return devices;
        }

        int numDevices = Pa_GetDeviceCount();
        for (int i = 0; i < numDevices; i++) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxOutputChannels > 0) {
                std::string name = info->name;
                if (i == Pa_GetDefaultOutputDevice()) {
                    name += " (Default)";
                }
                devices.push_back(name);
            }
        }

        if (devices.empty()) {
            devices.push_back("Default");
        }

        return devices;
    }

    bool AudioPlaybackSystem::SetOutputDevice(int deviceIndex) {
        if (!m_paInitialized) {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_playbackMutex);

        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }

        m_paDeviceIndex = deviceIndex;

        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_paDeviceIndex);
        if (deviceInfo) {
            m_deviceSampleRate = static_cast<int>(deviceInfo->defaultSampleRate);
            if (m_streamSampleRate == 0) {
                m_streamSampleRate = m_deviceSampleRate;
            }
            if (m_streamChannels == 0) {
                m_streamChannels = 2;
            }
            if (m_streamChannels > deviceInfo->maxOutputChannels) {
                m_streamChannels = deviceInfo->maxOutputChannels;
            }
        }

        PaStreamParameters outputParameters;
        outputParameters.device = m_paDeviceIndex;
        outputParameters.channelCount = m_streamChannels;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(m_paDeviceIndex)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&m_paStream,
            nullptr,
            &outputParameters,
            m_streamSampleRate,
            m_streamFramesPerBuffer,
            paClipOff,
            nullptr,
            nullptr);

        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        err = Pa_StartStream(m_paStream);
        if (err != paNoError) {
            std::cerr << "[AudioPlaybackSystem] Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }

        return true;
    }

    void AudioPlaybackSystem::AudioPlaybackThread() {
        std::cout << "[AudioPlaybackSystem] Playback thread started" << std::endl;

        const size_t MAX_INPUT_FRAMES = 4096;
        const size_t MAX_OUTPUT_FRAMES = MAX_INPUT_FRAMES * 2;
        m_resampleInput.resize(MAX_INPUT_FRAMES * m_streamChannels);
        m_resampleOutput.resize(MAX_OUTPUT_FRAMES * m_streamChannels);

        while (m_playbackThreadRunning) {
            std::unique_lock<std::mutex> cvLock(m_playbackCV_mutex);
            m_playbackCV.wait(cvLock, [this] { return m_playbackActive || !m_playbackThreadRunning; });
            if (!m_playbackThreadRunning) break;

            // Main playback loop
            while (true) {
                // Lock to check state and get current entity ID
                std::unique_lock<std::mutex> lock(m_playbackMutex);
                if (!m_playbackActive || m_playbackPaused || m_playingEntity == 0) {
                    // If we are paused or inactive, wait for resume or new play command
                    break;
                }

                EntityID entity = m_playingEntity;
                float speedRatio = m_playbackSpeedRatio;

                // Unlock to avoid holding while reading component and doing I/O
                lock.unlock();

                if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
                    // Entity invalid, stop playback
                    std::lock_guard<std::mutex> lock2(m_playbackMutex);
                    m_playbackActive = false;
                    m_playingEntity = 0;
                    break;
                }

                auto& audioComp = mgr.GetComponent<AudioComponent>(entity);

                // Check if audio data is ready
                if (audioComp.pcmData.empty() || !audioComp.isPlaying) {
                    std::lock_guard<std::mutex> lock2(m_playbackMutex);
                    m_playbackActive = false;
                    m_playingEntity = 0;
                    break;
                }

                int channels = audioComp.channels;
                const float* data = audioComp.pcmData.data();
                size_t totalSamples = audioComp.pcmData.size();
                size_t currentIdx = audioComp.currentSampleIndex;

                // Determine how many output frames we need
                int outputFrames = m_streamFramesPerBuffer;
                size_t outputSamples = outputFrames * m_streamChannels;

                // Calculate required input frames based on speed ratio
                double inputFramesNeeded = outputFrames * speedRatio;
                size_t inputSamplesNeeded = static_cast<size_t>(std::ceil(inputFramesNeeded * channels));

                // Check remaining samples
                size_t remainingSamples = totalSamples - currentIdx;
                if (remainingSamples == 0) {
                    if (audioComp.looping) {
                        // Loop: reset and continue
                        std::lock_guard<std::mutex> lock2(m_playbackMutex);
                        audioComp.currentSampleIndex = 0;
                        audioComp.currentTime = 0.0;
                        if (m_srcState) src_reset(m_srcState);
                        continue;
                    }
                    else {
                        std::lock_guard<std::mutex> lock2(m_playbackMutex);
                        audioComp.isPlaying = false;
                        m_playbackActive = false;
                        m_playingEntity = 0;
                        break;
                    }
                }

                size_t samplesToRead = std::min(inputSamplesNeeded, remainingSamples);
                if (samplesToRead == 0) {
                    // Not enough data, but we might still be able to produce some output; yield
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                // Prepare input buffer
                m_resampleInput.resize(samplesToRead);
                std::memcpy(m_resampleInput.data(), &data[currentIdx], samplesToRead * sizeof(float));

                // Prepare resampler data
                SRC_DATA srcData;
                srcData.data_in = m_resampleInput.data();
                srcData.input_frames = samplesToRead / channels;
                srcData.data_out = m_resampleOutput.data();
                srcData.output_frames = outputFrames * 2; // enough space
                srcData.src_ratio = speedRatio;
                srcData.end_of_input = 0;

                // --- CRITICAL FIX: Protect src_process with mutex to prevent concurrent src_reset ---
                {
                    std::lock_guard<std::mutex> lock2(m_playbackMutex);
                    int srcError = src_process(m_srcState, &srcData);
                    if (srcError != 0) {
                        std::cerr << "[AudioPlaybackSystem] Resampling error: " << src_strerror(srcError) << std::endl;
                        srcData.output_frames_gen = 0;
                    }
                }

                int framesGenerated = srcData.output_frames_gen;
                if (framesGenerated == 0) {
                    // No output generated yet; continue to next iteration without advancing index
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                // Apply volume and clamp
                float volume = audioComp.volume;
                size_t generatedSamples = framesGenerated * m_streamChannels;
                for (size_t i = 0; i < generatedSamples; ++i) {
                    float sample = m_resampleOutput[i] * volume;
                    if (sample > 1.0f) sample = 1.0f;
                    if (sample < -1.0f) sample = -1.0f;
                    m_resampleOutput[i] = sample;
                }

                // Write to stream
                PaError err = Pa_WriteStream(m_paStream, m_resampleOutput.data(), framesGenerated);
                if (err != paNoError) {
                    if (err == paOutputUnderflowed) {
                        // Underflow is normal if we are slow; just log and continue
                        std::cerr << "[AudioPlaybackSystem] Pa_WriteStream underflowed" << std::endl;
                    }
                    else {
                        std::cerr << "[AudioPlaybackSystem] Pa_WriteStream error: " << Pa_GetErrorText(err) << std::endl;
                        std::lock_guard<std::mutex> lock2(m_playbackMutex);
                        m_playbackActive = false;
                        m_playingEntity = 0;
                        break;
                    }
                }

                // Update position and time (lock mutex before writing to component)
                {
                    std::lock_guard<std::mutex> lock2(m_playbackMutex);
                    size_t framesConsumed = srcData.input_frames_used;
                    size_t samplesConsumed = framesConsumed * channels;
                    audioComp.currentSampleIndex += samplesConsumed;
                    audioComp.currentTime = static_cast<double>(audioComp.currentSampleIndex) / channels / audioComp.sampleRate;

                    // Notify callbacks
                    for (const auto& cb : m_callbacks) {
                        cb(entity, m_resampleOutput.data(), generatedSamples, m_streamChannels, m_streamSampleRate);
                    }

                    // Check if we've reached the end
                    if (audioComp.currentSampleIndex >= totalSamples) {
                        if (audioComp.looping) {
                            audioComp.currentSampleIndex = 0;
                            audioComp.currentTime = 0.0;
                            if (m_srcState) src_reset(m_srcState);
                        }
                        else {
                            audioComp.isPlaying = false;
                            m_playbackActive = false;
                            m_playingEntity = 0;
                        }
                    }
                }
            }
        }
        std::cout << "[AudioPlaybackSystem] Playback thread stopped" << std::endl;
    }

    void AudioPlaybackSystem::PlayTestTone() {
        std::cout << "[AudioPlaybackSystem] Generating test tone..." << std::endl;
        const int sampleRate = 44100;
        const int channels = 2;
        const int duration = 2;
        const int totalSamples = sampleRate * duration * channels;
        std::vector<float> testData(totalSamples);
        for (int i = 0; i < sampleRate * duration; ++i) {
            float sample = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / sampleRate);
            testData[i * 2] = sample;
            testData[i * 2 + 1] = sample;
        }

        EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<AudioComponent>(entity);
        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.pcmData = std::move(testData);
        audioComp.channels = 2;
        audioComp.sampleRate = 44100;
        audioComp.totalSamples = audioComp.pcmData.size() / 2;
        audioComp.duration = static_cast<double>(audioComp.totalSamples) / sampleRate;
        audioComp.fileName = "Test Tone";
        entities.insert(entity);

        m_streamSampleRate = 44100;
        m_streamChannels = 2;
        ReopenStream();
        Play(entity, false);
    }

} // namespace ECS