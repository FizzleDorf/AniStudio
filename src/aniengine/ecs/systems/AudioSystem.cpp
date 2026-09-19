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

    void AudioSystem::RegisterAudioAddedCallback(void* owner, const AudioCallback& callback) {
        audioAddedCallbacks.emplace_back(owner, callback);
    }

    void AudioSystem::RegisterAudioRemovedCallback(void* owner, const AudioCallback& callback) {
        audioRemovedCallbacks.emplace_back(owner, callback);
    }

    void AudioSystem::RegisterAudioDataCallback(void* owner, const AudioDataCallback& callback) {
        audioDataCallbacks.emplace_back(owner, callback);
    }

    void AudioSystem::UnregisterCallbacksForOwner(void* owner) {
        audioAddedCallbacks.erase(
            std::remove_if(audioAddedCallbacks.begin(), audioAddedCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            audioAddedCallbacks.end());
        audioRemovedCallbacks.erase(
            std::remove_if(audioRemovedCallbacks.begin(), audioRemovedCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            audioRemovedCallbacks.end());
        audioDataCallbacks.erase(
            std::remove_if(audioDataCallbacks.begin(), audioDataCallbacks.end(),
                [owner](const auto& p) { return p.first == owner; }),
            audioDataCallbacks.end());
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
        audioComp.isLoading = true;
        LoadAudioAsync(entity, filePath);
    }

    void AudioSystem::AddLoadedAudio(EntityID entity) {
        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            entities.insert(entity);
            auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            if (!audioComp.pcmData.empty()) {
                NotifyAudioAdded(entity);
                NotifyAudioData(entity, audioComp.pcmData.data(),
                    audioComp.pcmData.size(), audioComp.channels, audioComp.sampleRate);
            }
        }
    }

    AudioSystem::LoadResult AudioSystem::ExtractAudioFromVideoFile(const std::string& filePath, EntityID entity) {
        LoadResult result;
        result.filePath = filePath;
        result.entityID = entity;
        size_t lastSlash = filePath.find_last_of("/\\");
        result.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) + "_audio" : "audio";

        int targetRate = 44100;
        int targetChannels = 2;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
            return result;
        }
        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
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
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVCodecParameters* codecPar = fmtCtx->streams[audioStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) {
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
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVChannelLayout srcLayout, dstLayout;
        if (!av_channel_layout_check(&codecCtx->ch_layout)) {
            if (av_channel_layout_check(&codecPar->ch_layout))
                av_channel_layout_copy(&codecCtx->ch_layout, &codecPar->ch_layout);
            else if (codecPar->ch_layout.nb_channels > 0)
                av_channel_layout_default(&codecCtx->ch_layout, codecPar->ch_layout.nb_channels);
            else
                av_channel_layout_default(&codecCtx->ch_layout, 2);
        }
        av_channel_layout_copy(&srcLayout, &codecCtx->ch_layout);
        av_channel_layout_default(&dstLayout, targetChannels);

        SwrContext* swrCtx = swr_alloc();
        if (!swrCtx) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        av_opt_set_chlayout(swrCtx, "in_chlayout", &srcLayout, 0);
        av_opt_set_chlayout(swrCtx, "out_chlayout", &dstLayout, 0);
        av_opt_set_int(swrCtx, "in_sample_rate", codecCtx->sample_rate, 0);
        av_opt_set_int(swrCtx, "out_sample_rate", targetRate, 0);
        av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", codecCtx->sample_fmt, 0);
        av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

        if (swr_init(swrCtx) < 0) {
            swr_free(&swrCtx);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        int dstCh = dstLayout.nb_channels;
        std::vector<float> allPcm;

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        if (!frame || !pkt) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            swr_free(&swrCtx);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        while (av_read_frame(fmtCtx, pkt) >= 0) {
            if (pkt->stream_index != audioStream) {
                av_packet_unref(pkt);
                continue;
            }
            if (avcodec_send_packet(codecCtx, pkt) == 0) {
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    int numSamples = frame->nb_samples;
                    if (numSamples > 0) {
                        int maxOut = numSamples * 2;
                        uint8_t* outBuf = (uint8_t*)av_malloc(maxOut * dstCh * sizeof(float));
                        if (outBuf) {
                            int conv = swr_convert(swrCtx, &outBuf, maxOut,
                                (const uint8_t**)frame->data, frame->nb_samples);
                            if (conv > 0) {
                                float* fdata = (float*)outBuf;
                                size_t oldSize = allPcm.size();
                                allPcm.resize(oldSize + conv * dstCh);
                                std::memcpy(allPcm.data() + oldSize, fdata, conv * dstCh * sizeof(float));
                            }
                            av_free(outBuf);
                        }
                    }
                    av_frame_unref(frame);
                }
            }
            av_packet_unref(pkt);
        }

        int maxOut = 8192;
        uint8_t* flushBuf = (uint8_t*)av_malloc(maxOut * dstCh * sizeof(float));
        if (flushBuf) {
            int conv = swr_convert(swrCtx, &flushBuf, maxOut, nullptr, 0);
            if (conv > 0) {
                float* fdata = (float*)flushBuf;
                size_t oldSize = allPcm.size();
                allPcm.resize(oldSize + conv * dstCh);
                std::memcpy(allPcm.data() + oldSize, fdata, conv * dstCh * sizeof(float));
            }
            av_free(flushBuf);
        }

        av_packet_free(&pkt);
        av_frame_free(&frame);
        swr_free(&swrCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);

        if (allPcm.empty()) return result;

        result.success = true;
        result.pcmData = std::move(allPcm);
        result.channels = dstCh;
        result.sampleRate = targetRate;
        result.totalSamples = result.pcmData.size() / dstCh;
        result.duration = static_cast<double>(result.totalSamples) / targetRate;

        return result;
    }

    void AudioSystem::RemoveAudio(EntityID entity) {
        if (!mgr.HasComponent<AudioComponent>(entity)) {
            return;
        }

        auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
        audioComp.UnloadAudio();

        entities.erase(entity);
        NotifyAudioRemoved(entity);
    }

    void AudioSystem::ClearCache(EntityID entity) {
        {
            std::lock_guard<std::mutex> lock(loadMutex);
            pendingLoads.erase(
                std::remove_if(pendingLoads.begin(), pendingLoads.end(),
                    [entity](const LoadingTask& t) { return t.entityID == entity; }),
                pendingLoads.end());
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<AudioComponent>(entity)) {
            auto& ac = mgr.GetComponent<AudioComponent>(entity);
            ac.UnloadAudio();
            ac.isLoading = false;
            ac.currentTime = 0.0;
            ac.reachedEnd = false;
        }

        entities.erase(entity);
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

        if (!av_channel_layout_check(&codecCtx->ch_layout) && av_channel_layout_check(&codecPar->ch_layout)) {
            av_channel_layout_copy(&codecCtx->ch_layout, &codecPar->ch_layout);
        }
        if (!av_channel_layout_check(&codecCtx->ch_layout) && codecPar->ch_layout.nb_channels > 0) {
            av_channel_layout_default(&codecCtx->ch_layout, codecPar->ch_layout.nb_channels);
        }
        int srcChannels = codecCtx->ch_layout.nb_channels;
        if (srcChannels == 0) {
            srcChannels = codecPar->ch_layout.nb_channels;
            if (srcChannels == 0) srcChannels = 2;
            av_channel_layout_default(&codecCtx->ch_layout, srcChannels);
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

        SwrContext* swrCtx = swr_alloc();
        if (!swrCtx) {
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVChannelLayout inLayout;
        av_channel_layout_copy(&inLayout, &codecCtx->ch_layout);
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, targetChannels);

        av_opt_set_chlayout(swrCtx, "in_chlayout", &inLayout, 0);
        av_opt_set_chlayout(swrCtx, "out_chlayout", &outLayout, 0);
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

        int dstChannels = outLayout.nb_channels;

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
                        uint8_t* outBuffer = (uint8_t*)av_malloc(maxOutSamples * dstChannels * sizeof(float));

                        if (outBuffer) {
                            int convertedSamples = swr_convert(swrCtx, &outBuffer, maxOutSamples,
                                (const uint8_t**)frame->data, frame->nb_samples);

                            if (convertedSamples > 0) {
                                float* floatData = (float*)outBuffer;
                                for (int i = 0; i < convertedSamples * dstChannels; ++i) {
                                    if (floatData[i] < -1.0f) floatData[i] = -1.0f;
                                    if (floatData[i] > 1.0f) floatData[i] = 1.0f;
                                }
                                size_t startIndex = allPcmData.size();
                                allPcmData.resize(startIndex + convertedSamples * dstChannels);
                                std::memcpy(allPcmData.data() + startIndex, floatData,
                                    convertedSamples * dstChannels * sizeof(float));
                            }

                            av_free(outBuffer);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }

        int maxOutSamples = 8192;
        uint8_t* flushBuffer = (uint8_t*)av_malloc(maxOutSamples * dstChannels * sizeof(float));
        if (flushBuffer) {
            int flushSamples = swr_convert(swrCtx, &flushBuffer, maxOutSamples, nullptr, 0);
            if (flushSamples > 0) {
                float* floatData = (float*)flushBuffer;
                for (int i = 0; i < flushSamples * dstChannels; ++i) {
                    if (floatData[i] < -1.0f) floatData[i] = -1.0f;
                    if (floatData[i] > 1.0f) floatData[i] = 1.0f;
                }
                size_t startIndex = allPcmData.size();
                allPcmData.resize(startIndex + flushSamples * dstChannels);
                std::memcpy(allPcmData.data() + startIndex, floatData,
                    flushSamples * dstChannels * sizeof(float));
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
        result.channels = dstChannels;
        result.sampleRate = targetSampleRate;
        result.totalSamples = result.pcmData.size() / dstChannels;
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
                            audioComp.isLoading = false;

                            NotifyAudioAdded(result.entityID);
                            NotifyAudioData(result.entityID,
                                audioComp.pcmData.data(),
                                audioComp.pcmData.size(),
                                audioComp.channels,
                                audioComp.sampleRate);
                        }
                        else {
                            audioComp.isLoading = false;
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
        for (const auto& [owner, cb] : audioAddedCallbacks) {
            (void)owner;
            try { cb(entity); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio added callback: " << e.what() << std::endl;
            }
        }
    }

    void AudioSystem::NotifyAudioRemoved(EntityID entity) {
        for (const auto& [owner, cb] : audioRemovedCallbacks) {
            (void)owner;
            try { cb(entity); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio removed callback: " << e.what() << std::endl;
            }
        }
    }

    void AudioSystem::NotifyAudioData(EntityID entity, const float* data, size_t size, int channels, int sampleRate) {
        for (const auto& [owner, cb] : audioDataCallbacks) {
            (void)owner;
            try { cb(entity, data, size, channels, sampleRate); }
            catch (const std::exception& e) {
                std::cerr << "[AudioSystem] Exception in audio data callback: " << e.what() << std::endl;
            }
        }
    }

}