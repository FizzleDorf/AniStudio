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

        std::cout << "[AudioSystem] Initialized (data loading only)" << std::endl;
    }

    AudioSystem::~AudioSystem() {
        std::cout << "[AudioSystem] Destructor - cleaning up" << std::endl;

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

    AudioSystem::LoadingTask::LoadingTask(LoadingTask&& other) noexcept
        : entityID(other.entityID)
        , filePath(std::move(other.filePath))
        , future(std::move(other.future)) {
    }

    AudioSystem::LoadingTask& AudioSystem::LoadingTask::operator=(LoadingTask&& other) noexcept {
        if (this != &other) {
            entityID = other.entityID;
            filePath = std::move(other.filePath);
            future = std::move(other.future);
        }
        return *this;
    }

    void AudioSystem::Start() {
        std::cout << "[AudioSystem] Started" << std::endl;

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
    }

    void AudioSystem::Destroy() {
        std::cout << "[AudioSystem] Destroying" << std::endl;

        for (auto entity : entities) {
            if (mgr.HasComponent<AudioComponent>(entity)) {
                auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
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

        int targetRate = 44100;
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