#include "AudioSystem.hpp"
#include "AudioUtils.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>

namespace ECS {

    AudioSystem::AudioSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "AudioSystem";
        AddComponentSignature<AudioComponent>();

        avdevice_register_all();

        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "[AudioSystem] Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            m_paInitialized = true;
            std::cout << "[AudioSystem] PortAudio initialized" << std::endl;

            int numDevices = Pa_GetDeviceCount();
            std::cout << "[AudioSystem] Found " << numDevices << " audio devices" << std::endl;
            for (int i = 0; i < numDevices; i++) {
                const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
                if (info) {
                    std::cout << "  Device " << i << ": " << info->name
                        << " (max out channels: " << info->maxOutputChannels << ")" << std::endl;
                }
            }

            m_paDeviceIndex = Pa_GetDefaultOutputDevice();
            std::cout << "[AudioSystem] Default output device: " << m_paDeviceIndex << std::endl;
            if (m_paDeviceIndex >= 0) {
                const PaDeviceInfo* info = Pa_GetDeviceInfo(m_paDeviceIndex);
                if (info) {
                    m_deviceSampleRate = static_cast<int>(info->defaultSampleRate);
                    std::cout << "[AudioSystem] Device default sample rate: " << m_deviceSampleRate << " Hz" << std::endl;
                }
            }
        }

        std::cout << "[AudioSystem] Initialized with FFmpeg audio support" << std::endl;
    }

    AudioSystem::~AudioSystem() {
        std::cout << "[AudioSystem] Destructor - cleaning up" << std::endl;

        m_playbackActive = false;
        if (m_playbackThread.joinable()) {
            m_playbackCV.notify_one();
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

        for (auto entity : entities) {
            if (mgr.HasComponent<AudioComponent>(entity)) {
                auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
                audioComp.UnloadAudio();
            }
        }
    }

    void AudioSystem::ReopenStream() {
        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }

        if (!m_paInitialized || m_paDeviceIndex < 0) {
            return;
        }

        if (m_streamSampleRate == 0 || m_streamChannels == 0) {
            if (!entities.empty()) {
                EntityID firstEntity = *entities.begin();
                if (mgr.IsEntityValid(firstEntity) && mgr.HasComponent<AudioComponent>(firstEntity)) {
                    auto& audioComp = mgr.GetComponent<AudioComponent>(firstEntity);
                    if (audioComp.sampleRate > 0) {
                        m_streamSampleRate = audioComp.sampleRate;
                        m_streamChannels = audioComp.channels;
                    }
                }
            }
            if (m_streamSampleRate == 0) m_streamSampleRate = 44100;
            if (m_streamChannels == 0) m_streamChannels = 2;
        }

        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(m_paDeviceIndex);
        if (deviceInfo && m_streamChannels > deviceInfo->maxOutputChannels) {
            m_streamChannels = deviceInfo->maxOutputChannels;
        }

        std::cout << "[AudioSystem] Opening stream: " << m_streamSampleRate << "Hz, "
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
            nullptr, // no callback
            nullptr);

        if (err != paNoError) {
            std::cerr << "[AudioSystem] Failed to open PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
            return;
        }

        const PaStreamInfo* streamInfo = Pa_GetStreamInfo(m_paStream);
        if (streamInfo) {
            m_actualStreamSampleRate = static_cast<int>(streamInfo->sampleRate);
            std::cout << "[AudioSystem] Actual stream sample rate: " << m_actualStreamSampleRate << " Hz" << std::endl;
            if (m_actualStreamSampleRate != m_streamSampleRate) {
                std::cout << "[AudioSystem] WARNING: Stream opened at different sample rate. Adjusting to match." << std::endl;
                m_streamSampleRate = m_actualStreamSampleRate;
            }
        }

        err = Pa_StartStream(m_paStream);
        if (err != paNoError) {
            std::cerr << "[AudioSystem] Failed to start PortAudio stream: " << Pa_GetErrorText(err) << std::endl;
        }
        else {
            m_streamInitialized = true;
            std::cout << "[AudioSystem] PortAudio stream started at " << m_streamSampleRate << " Hz" << std::endl;
        }
    }

    void AudioSystem::Start() {
        std::cout << "[AudioSystem] Started" << std::endl;

        if (!m_playbackThreadRunning) {
            m_playbackThreadRunning = true;
            m_playbackThread = std::thread(&AudioSystem::AudioPlaybackThread, this);
        }

        auto allEntities = mgr.GetAllEntities();
        for (auto entity : allEntities) {
            if (mgr.HasComponent<AudioComponent>(entity)) {
                entities.insert(entity);
                auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
                if (!audioComp.filePath.empty()) {
                    LoadAudioAsync(entity, audioComp.filePath);
                }
            }
        }
    }

    void AudioSystem::Update(float deltaT) {
        ProcessCompletedLoads();

        if (m_streamNeedsReopen && !pendingLoads.empty()) {
            ReopenStream();
            m_streamNeedsReopen = false;
        }

        // No need to update playback here; the thread handles it.
    }

    void AudioSystem::Destroy() {
        std::cout << "[AudioSystem] Destroying" << std::endl;

        m_playbackActive = false;
        if (m_playbackThread.joinable()) {
            m_playbackCV.notify_one();
            m_playbackThread.join();
        }

        if (m_paStream) {
            Pa_StopStream(m_paStream);
            Pa_CloseStream(m_paStream);
            m_paStream = nullptr;
        }

        for (auto entity : entities) {
            if (mgr.HasComponent<AudioComponent>(entity)) {
                auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
                audioComp.isPlaying = false;
                audioComp.UnloadAudio();
            }
        }
    }

    void AudioSystem::RegisterAudioAddedCallback(const AudioCallback& callback) {
        audioAddedCallbacks.push_back(callback);
    }

    void AudioSystem::RegisterAudioRemovedCallback(const AudioCallback& callback) {
        audioRemovedCallbacks.push_back(callback);
    }

    void AudioSystem::RegisterAudioDataCallback(const AudioDataCallback& callback) {
        audioDataCallbacks.push_back(callback);
    }

    void AudioSystem::SetAudio(EntityID entity, const std::string& filePath) {
        if (!mgr.HasComponent<AudioComponent>(entity)) {
            std::cerr << "[AudioSystem] Entity " << entity << " does not have AudioComponent" << std::endl;
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.UnloadAudio();
        audioComp.filePath = filePath;

        size_t lastSlash = filePath.find_last_of("/\\");
        audioComp.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

        entities.insert(entity);
        LoadAudioAsync(entity, filePath);
        m_streamNeedsReopen = true;
    }

    void AudioSystem::RemoveAudio(EntityID entity) {
        if (!mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.isPlaying = false;
        audioComp.UnloadAudio();

        entities.erase(entity);
        NotifyAudioRemoved(entity);
        mgr.DestroyEntity(entity);
    }

    std::vector<EntityID> AudioSystem::GetAllAudioEntities() const {
        std::vector<EntityID> result;
        for (auto entity : entities) {
            if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
                result.push_back(entity);
            }
        }
        return result;
    }

    void AudioSystem::Play(EntityID entity, bool loop) {
        std::lock_guard<std::mutex> lock(playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);

        if (audioComp.pcmData.empty()) {
            std::cerr << "[AudioSystem] No audio data loaded for entity " << entity << std::endl;
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

        m_playbackCV.notify_one();

        std::cout << "[AudioSystem] Playing audio: " << audioComp.fileName << std::endl;
        std::cout << "[AudioSystem] PCM total samples: " << audioComp.pcmData.size()
            << ", channels: " << audioComp.channels
            << ", first 4 samples: " << audioComp.pcmData[0] << ", "
            << audioComp.pcmData[1] << ", " << audioComp.pcmData[2] << ", " << audioComp.pcmData[3] << std::endl;
    }

    void AudioSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.isPlaying = false;
        audioComp.isPaused = false;
        audioComp.currentSampleIndex = 0;
        audioComp.currentTime = 0.0;

        if (m_playingEntity == entity) {
            m_playbackActive = false;
            m_playingEntity = 0;
        }
    }

    void AudioSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        if (audioComp.isPlaying) {
            audioComp.isPaused = true;
            m_playbackPaused = true;
        }
    }

    void AudioSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(playbackMutex);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        if (audioComp.isPlaying && audioComp.isPaused) {
            audioComp.isPaused = false;
            m_playbackPaused = false;
            m_playbackCV.notify_one();
        }
    }

    void AudioSystem::SetVolume(EntityID entity, float volume) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.volume = std::clamp(volume, 0.0f, 1.0f);
    }

    void AudioSystem::Seek(EntityID entity, double position) {
        std::lock_guard<std::mutex> lock(playbackMutex);

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
    }

    double AudioSystem::GetCurrentPosition(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return 0.0;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.currentTime;
    }

    double AudioSystem::GetDuration(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return 0.0;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.duration;
    }

    bool AudioSystem::IsPlaying(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return false;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.isPlaying && !audioComp.isPaused;
    }

    bool AudioSystem::IsPaused(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            return false;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        return audioComp.isPlaying && audioComp.isPaused;
    }

    const float* AudioSystem::GetAudioData(EntityID entity, size_t& outSize, int& outChannels) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<AudioComponent>(entity)) {
            outSize = 0;
            outChannels = 0;
            return nullptr;
        }

        const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        outSize = audioComp.pcmData.size();
        outChannels = audioComp.channels;
        return audioComp.pcmData.data();
    }

    void AudioSystem::LoadAudioAsync(EntityID entity, const std::string& filePath) {
        auto threadPoolSys = mgr.GetSystem<ThreadPoolSystem>();
        if (!threadPoolSys) {
            std::cerr << "[AudioSystem] ThreadPoolSystem not available!" << std::endl;
            return;
        }

        auto& ioPool = threadPoolSys->getIOPool();

        int targetRate = m_actualStreamSampleRate > 0 ? m_actualStreamSampleRate : (m_deviceSampleRate > 0 ? m_deviceSampleRate : 44100);
        int targetChannels = 2;

        auto future = ioPool.submit([filePath, entity, targetRate, targetChannels]() -> LoadResult {
            return DecodeAudioFile(filePath, entity, targetRate, targetChannels);
            });

        std::lock_guard<std::mutex> lock(loadMutex);
        LoadingTask task;
        task.entityID = entity;
        task.filePath = filePath;
        task.future = std::move(future);
        pendingLoads.push_back(std::move(task));
    }

    AudioSystem::LoadResult AudioSystem::DecodeAudioFile(const std::string& filePath, EntityID entity, int targetSampleRate, int targetChannels) {
        LoadResult result;
        result.filePath = filePath;
        result.entityID = entity;

        size_t lastSlash = filePath.find_last_of("/\\");
        result.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "[AudioSystem] Failed to open audio file: " << filePath << std::endl;
            return result;
        }

        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            std::cerr << "[AudioSystem] Failed to find stream info: " << filePath << std::endl;
            avformat_close_input(&fmtCtx);
            return result;
        }

        int audioStream = -1;
        for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audioStream = i;
                break;
            }
        }

        if (audioStream == -1) {
            std::cerr << "[AudioSystem] No audio stream found: " << filePath << std::endl;
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVCodecParameters* codecPar = fmtCtx->streams[audioStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) {
            std::cerr << "[AudioSystem] Codec not found: " << filePath << std::endl;
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) {
            avformat_close_input(&fmtCtx);
            return result;
        }

        if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        if (codecCtx->channels == 0 && codecPar->channels > 0) {
            codecCtx->channels = codecPar->channels;
        }

        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        if (!frame || !pkt) {
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        std::cout << "[AudioSystem] Source: " << codecCtx->sample_rate << "Hz, "
            << codecCtx->channels << "ch, target: " << targetSampleRate << "Hz, "
            << targetChannels << "ch" << std::endl;

        SwrContext* swrCtx = swr_alloc();
        if (!swrCtx) {
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        uint64_t sourceChannelLayout = codecCtx->channel_layout;
        if (sourceChannelLayout == 0 && codecCtx->channels > 0) {
            sourceChannelLayout = av_get_default_channel_layout(codecCtx->channels);
        }

        uint64_t targetChannelLayout = (targetChannels == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;

        av_opt_set_int(swrCtx, "in_channel_layout", sourceChannelLayout, 0);
        av_opt_set_int(swrCtx, "out_channel_layout", targetChannelLayout, 0);
        av_opt_set_int(swrCtx, "in_sample_rate", codecCtx->sample_rate, 0);
        av_opt_set_int(swrCtx, "out_sample_rate", targetSampleRate, 0);
        av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", codecCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        av_opt_set_int(swrCtx, "exact_rational", 1, 0);
        av_opt_set_int(swrCtx, "filter_size", 16, 0);
        av_opt_set_int(swrCtx, "phase_shift", 10, 0);

        if (swr_init(swrCtx) < 0) {
            std::cerr << "[AudioSystem] Failed to initialize swr context" << std::endl;
            swr_free(&swrCtx);
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        std::vector<float> allPcmData;

        while (av_read_frame(fmtCtx, pkt) >= 0) {
            if (pkt->stream_index != audioStream) {
                av_packet_unref(pkt);
                continue;
            }

            if (avcodec_send_packet(codecCtx, pkt) == 0) {
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    int numSamples = frame->nb_samples;

                    if (numSamples > 0) {
                        int maxOutSamples = numSamples * 2;
                        uint8_t* outBuffer = (uint8_t*)av_malloc(maxOutSamples * targetChannels * sizeof(float));

                        if (outBuffer) {
                            int convertedSamples = swr_convert(swrCtx, &outBuffer, maxOutSamples,
                                (const uint8_t**)frame->data, frame->nb_samples);

                            if (convertedSamples > 0) {
                                float* floatData = (float*)outBuffer;
                                for (int i = 0; i < convertedSamples * targetChannels; ++i) {
                                    if (floatData[i] < -1.0f) floatData[i] = -1.0f;
                                    if (floatData[i] > 1.0f) floatData[i] = 1.0f;
                                }
                                size_t startIndex = allPcmData.size();
                                allPcmData.resize(startIndex + convertedSamples * targetChannels);
                                std::memcpy(allPcmData.data() + startIndex, floatData,
                                    convertedSamples * targetChannels * sizeof(float));
                            }

                            av_free(outBuffer);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }

        int maxOutSamples = 8192;
        uint8_t* flushBuffer = (uint8_t*)av_malloc(maxOutSamples * targetChannels * sizeof(float));
        if (flushBuffer) {
            int flushSamples = swr_convert(swrCtx, &flushBuffer, maxOutSamples, nullptr, 0);
            if (flushSamples > 0) {
                float* floatData = (float*)flushBuffer;
                for (int i = 0; i < flushSamples * targetChannels; ++i) {
                    if (floatData[i] < -1.0f) floatData[i] = -1.0f;
                    if (floatData[i] > 1.0f) floatData[i] = 1.0f;
                }
                size_t startIndex = allPcmData.size();
                allPcmData.resize(startIndex + flushSamples * targetChannels);
                std::memcpy(allPcmData.data() + startIndex, floatData,
                    flushSamples * targetChannels * sizeof(float));
            }
            av_free(flushBuffer);
        }

        swr_free(&swrCtx);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);

        if (allPcmData.empty()) {
            std::cerr << "[AudioSystem] No PCM data decoded for: " << filePath << std::endl;
            return result;
        }

        result.success = true;
        result.pcmData = std::move(allPcmData);
        result.channels = targetChannels;
        result.sampleRate = targetSampleRate;
        result.totalSamples = result.pcmData.size() / targetChannels;
        result.duration = static_cast<double>(result.totalSamples) / targetSampleRate;

        try {
            result.hasExif = Utils::AudioUtils::HasExifMetadata(filePath);
            result.hasLSB = Utils::AudioUtils::HasLSBMetadata(filePath);
            result.hasAniStudio = Utils::AudioUtils::GetMetadataStatus(filePath) > 0;
        }
        catch (...) {}

        std::cout << "[AudioSystem] Decoded audio: " << filePath
            << " (" << result.channels << "ch, " << result.sampleRate << "Hz, "
            << result.duration << "s, " << result.pcmData.size() / 1024 / 1024 << "MB)" << std::endl;

        return result;
    }

    void AudioSystem::ProcessCompletedLoads() {
        std::lock_guard<std::mutex> lock(loadMutex);

        for (auto it = pendingLoads.begin(); it != pendingLoads.end();) {
            if (it->future.valid() &&
                it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {

                try {
                    LoadResult result = it->future.get();

                    if (mgr.HasComponent<AudioComponent>(result.entityID)) {
                        auto& audioComp = mgr.GetComponent<AudioComponent>(result.entityID);

                        if (result.success) {
                            audioComp.pcmData = std::move(result.pcmData);
                            audioComp.channels = result.channels;
                            audioComp.sampleRate = result.sampleRate;
                            audioComp.totalSamples = result.totalSamples;
                            audioComp.duration = result.duration;
                            audioComp.fileName = result.fileName;
                            audioComp.filePath = result.filePath;
                            audioComp.hasExifData = result.hasExif;
                            audioComp.hasLSBData = result.hasLSB;
                            audioComp.hasAniStudioMetadata = result.hasAniStudio;

                            if (m_streamSampleRate == 0 || m_streamChannels == 0) {
                                m_streamSampleRate = result.sampleRate;
                                m_streamChannels = result.channels;
                                m_streamNeedsReopen = true;
                            }

                            NotifyAudioAdded(result.entityID);
                            NotifyAudioData(result.entityID,
                                audioComp.pcmData.data(),
                                audioComp.pcmData.size(),
                                audioComp.channels,
                                audioComp.sampleRate);
                        }
                        else {
                            std::cerr << "[AudioSystem] Failed to load audio: " << result.filePath << std::endl;
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "[AudioSystem] Exception in ProcessCompletedLoads: " << e.what() << std::endl;
                }

                it = pendingLoads.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    // Blocking I/O thread function
    void AudioSystem::AudioPlaybackThread() {
        std::cout << "[AudioSystem] Playback thread started" << std::endl;
        while (m_playbackThreadRunning) {
            std::unique_lock<std::mutex> lock(m_playbackCV_mutex);
            m_playbackCV.wait(lock, [this] { return m_playbackActive || !m_playbackThreadRunning; });
            if (!m_playbackThreadRunning) break;

            // Playback loop
            while (m_playbackActive && !m_playbackPaused) {
                if (m_playingEntity == 0 || !mgr.IsEntityValid(m_playingEntity) || !mgr.HasComponent<AudioComponent>(m_playingEntity)) {
                    m_playbackActive = false;
                    break;
                }

                auto& audioComp = mgr.GetComponent<AudioComponent>(m_playingEntity);
                if (audioComp.pcmData.empty() || !audioComp.isPlaying) {
                    m_playbackActive = false;
                    break;
                }

                int channels = audioComp.channels;
                int sampleRate = audioComp.sampleRate;
                const float* data = audioComp.pcmData.data();
                size_t totalSamples = audioComp.pcmData.size();
                size_t currentIdx = audioComp.currentSampleIndex;

                // Calculate how many frames to write
                int framesToWrite = m_streamFramesPerBuffer;
                size_t samplesToWrite = framesToWrite * channels;
                if (currentIdx + samplesToWrite > totalSamples) {
                    samplesToWrite = totalSamples - currentIdx;
                    framesToWrite = samplesToWrite / channels;
                }

                if (samplesToWrite == 0) {
                    // End of stream
                    if (audioComp.looping) {
                        audioComp.currentSampleIndex = 0;
                        audioComp.currentTime = 0.0;
                        continue;
                    }
                    else {
                        audioComp.isPlaying = false;
                        m_playbackActive = false;
                        break;
                    }
                }

                // Prepare buffer with volume scaling and clamping
                std::vector<float> buffer(samplesToWrite);
                float volume = audioComp.volume;
                for (size_t i = 0; i < samplesToWrite; ++i) {
                    float sample = data[currentIdx + i] * volume;
                    if (sample > 1.0f) sample = 1.0f;
                    if (sample < -1.0f) sample = -1.0f;
                    buffer[i] = sample;
                }

                // Write to stream (blocking)
                PaError err = Pa_WriteStream(m_paStream, buffer.data(), framesToWrite);
                if (err != paNoError) {
                    std::cerr << "[AudioSystem] Pa_WriteStream error: " << Pa_GetErrorText(err) << std::endl;
                    m_playbackActive = false;
                    break;
                }

                // Update position
                audioComp.currentSampleIndex += samplesToWrite;
                audioComp.currentTime = static_cast<double>(audioComp.currentSampleIndex) / channels / sampleRate;

                // Notify data (optional)
                NotifyAudioData(m_playingEntity, buffer.data(), buffer.size(), channels, sampleRate);
            }

            // If we exit the loop, clear the active flag if not paused
            if (!m_playbackPaused) {
                m_playbackActive = false;
            }
        }
        std::cout << "[AudioSystem] Playback thread stopped" << std::endl;
    }

    void AudioSystem::PlayTestTone() {
        std::cout << "[AudioSystem] Generating test tone..." << std::endl;
        const int sampleRate = 44100;
        const int channels = 2;
        const int duration = 2;
        const int totalSamples = sampleRate * duration * channels;
        std::vector<float> testData(totalSamples);
        for (int i = 0; i < sampleRate * duration; ++i) {
            float sample = 0.5f * sin(2.0f * 3.14159f * 440.0f * i / sampleRate);
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

    void AudioSystem::NotifyAudioAdded(EntityID entity) {
        for (const auto& cb : audioAddedCallbacks) {
            try { cb(entity); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio added callback: " << e.what() << std::endl;
            }
        }
    }

    void AudioSystem::NotifyAudioRemoved(EntityID entity) {
        for (const auto& cb : audioRemovedCallbacks) {
            try { cb(entity); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio removed callback: " << e.what() << std::endl;
            }
        }
    }

    void AudioSystem::NotifyAudioData(EntityID entity, const float* data, size_t size, int channels, int sampleRate) {
        for (const auto& cb : audioDataCallbacks) {
            try { cb(entity, data, size, channels, sampleRate); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio data callback: " << e.what() << std::endl;
            }
        }
    }

} // namespace ECS