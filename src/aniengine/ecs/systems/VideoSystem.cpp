#include "VideoSystem.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace ECS {

    VideoSystem::VideoSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr), lastFrameTime(std::chrono::high_resolution_clock::now()) {
        sysName = "VideoSystem";
        AddComponentSignature<VideoComponent>();
    }

    VideoSystem::~VideoSystem() {
        for (auto entity : entities) {
            if (mgr.HasComponent<VideoComponent>(entity)) {
                auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                videoComp.UnloadVideo();
            }
        }
    }

    void VideoSystem::Start() {
        auto allEntities = mgr.GetAllEntities();
        for (auto entity : allEntities) {
            if (mgr.HasComponent<VideoComponent>(entity)) {
                entities.insert(entity);
                auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                if (!videoComp.filePath.empty()) {
                    LoadVideoAsync(entity, videoComp.filePath);
                }
            }
        }
    }

    void VideoSystem::Update(float deltaT) {
        ProcessCompletedLoads();
        ProcessCompletedSaves();
    }

    void VideoSystem::SetVideo(EntityID entity, const std::string& filePath) {
        if (mgr.HasComponent<VideoComponent>(entity)) {
            auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
            videoComp.UnloadVideo();
            videoComp.filePath = filePath;
            size_t lastSlash = filePath.find_last_of("/\\");
            videoComp.fileName = (lastSlash != std::string::npos) ?
                filePath.substr(lastSlash + 1) : filePath;
            entities.insert(entity);
            LoadVideoAsync(entity, filePath);
        }
    }

    void VideoSystem::RemoveVideo(EntityID entity) {
        if (mgr.HasComponent<VideoComponent>(entity)) {
            auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
            videoComp.UnloadVideo();
            entities.erase(entity);

            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveFutures.erase(entity);
            m_savePaths.erase(entity);
            std::lock_guard<std::mutex> lock2(m_loadMutex);
            m_loadingStatus.erase(entity);

            NotifyVideoRemoved(entity);
            mgr.DestroyEntity(entity);
        }
    }

    std::vector<EntityID> VideoSystem::GetAllVideoEntities() const {
        std::vector<EntityID> result;
        for (auto entity : entities) {
            if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity))
                result.push_back(entity);
        }
        return result;
    }

    void VideoSystem::RegisterVideoAddedCallback(const VideoCallback& cb) {
        videoAddedCallbacks.push_back(cb);
    }

    void VideoSystem::RegisterVideoRemovedCallback(const VideoCallback& cb) {
        videoRemovedCallbacks.push_back(cb);
    }

    void VideoSystem::RegisterVideoAudioCallback(const VideoAudioCallback& cb) {
        videoAudioCallbacks.push_back(cb);
    }

    void VideoSystem::RegisterSaveCallback(const SaveCallback& cb) {
        saveCallbacks.push_back(cb);
    }

    void VideoSystem::RegisterLoadCallback(const LoadCallback& cb) {
        loadCallbacks.push_back(cb);
    }

    void VideoSystem::SetVideoTextureCallback(const VideoTextureCallback& callback) {
        m_textureCallback = callback;
    }

    bool VideoSystem::SeekToFrame(VideoComponent& videoComp, long long frameIndex) {
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0 || frameIndex < 0 || frameIndex >= videoComp.frameCount)
            return false;

        double timeSec = static_cast<double>(frameIndex) / videoComp.fps;
        int64_t target_ts = (int64_t)(timeSec * AV_TIME_BASE);
        if (avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, target_ts, target_ts, 0) < 0) {
            avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, 0, 0, 0);
        }
        avcodec_flush_buffers(videoComp.codecCtx);

        videoComp.frameAccumulator = 0.0f;

        if (DecodeNextFrame(videoComp)) {
            if (m_textureCallback) {
                m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                    videoComp.width, videoComp.height, 4,
                    &videoComp.currentTexture);
            }
            videoComp.currentFrame = frameIndex;
            return true;
        }
        return false;
    }

    bool VideoSystem::AdvanceOneFrame(VideoComponent& videoComp) {
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0)
            return false;
        if (DecodeNextFrame(videoComp)) {
            if (m_textureCallback) {
                m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                    videoComp.width, videoComp.height, 4,
                    &videoComp.currentTexture);
            }
            return true;
        }
        return false;
    }

    void VideoSystem::UpdateMetadataFlags(EntityID entity) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity))
            return;
        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        if (videoComp.filePath.empty())
            return;
        try {
            videoComp.hasExifData = Utils::VideoUtils::HasExifMetadata(videoComp.filePath);
            videoComp.hasLSBData = Utils::VideoUtils::HasLSBMetadata(videoComp.filePath);
        }
        catch (...) {
            videoComp.hasExifData = false;
            videoComp.hasLSBData = false;
        }
    }

    bool VideoSystem::IsLoading(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_loadMutex);
        auto it = m_loadingStatus.find(entity);
        return it != m_loadingStatus.end() && it->second;
    }

    bool VideoSystem::IsSaving(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        auto it = m_saveFutures.find(entity);
        if (it == m_saveFutures.end()) return false;
        return it->second.valid() &&
            it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout;
    }

    void VideoSystem::SaveVideoAsync(EntityID entity, const std::string& outputPath) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            NotifySaveComplete(entity, false, "");
            return;
        }

        auto threadPool = mgr.GetSystem<ThreadPoolSystem>();
        if (!threadPool) {
            std::cerr << "[VideoSystem] ThreadPoolSystem not available!" << std::endl;
            NotifySaveComplete(entity, false, "");
            return;
        }

        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0) {
            std::cerr << "[VideoSystem] Video not loaded or invalid" << std::endl;
            NotifySaveComplete(entity, false, "");
            return;
        }

        SaveTaskData taskData;
        taskData.inputPath = videoComp.filePath;
        taskData.fps = static_cast<int>(videoComp.fps);
        taskData.width = videoComp.width;
        taskData.height = videoComp.height;
        taskData.frameCount = videoComp.frameCount;
        taskData.hasAudio = false;

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoAudioComponent>(entity)) {
            auto& videoAudioComp = mgr.GetComponent<VideoAudioComponent>(entity);
            if (videoAudioComp.hasAudio) {
                EntityID audioEntity = videoAudioComp.audioEntityID;
                if (mgr.IsEntityValid(audioEntity) && mgr.HasComponent<AudioComponent>(audioEntity)) {
                    auto& audioComp = mgr.GetComponent<AudioComponent>(audioEntity);
                    if (!audioComp.pcmData.empty()) {
                        taskData.audioPcmData = audioComp.pcmData;
                        taskData.audioChannels = audioComp.channels;
                        taskData.audioSampleRate = audioComp.sampleRate;
                        taskData.audioDuration = audioComp.duration;
                        taskData.hasAudio = true;
                        std::cout << "[VideoSystem] Copying audio data: " << taskData.audioPcmData.size() << " samples" << std::endl;
                    }
                }
            }
        }

        std::string savePath = outputPath;
        if (savePath.empty()) {
            std::string baseName = std::filesystem::path(videoComp.filePath).stem().string();
            std::string ext = ".webm";
            std::string dir = std::filesystem::path(videoComp.filePath).parent_path().string();
            savePath = dir + "/" + baseName + "_saved" + ext;
        }
        taskData.outputPath = savePath;

        std::cout << "[VideoSystem] Submitting save task for entity " << entity << " to " << savePath << std::endl;

        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_savePaths[entity] = savePath;
            m_saveFutures[entity] = threadPool->getIOPool().submit([this, entity, taskData]() -> bool {
                return SaveVideoInBackground(taskData);
                });
        }

        std::cout << "[VideoSystem] Started async save for entity " << entity << " to " << savePath << std::endl;
    }

    void VideoSystem::LoadVideoAsync(EntityID entity, const std::string& filePath) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            NotifyLoadComplete(entity, false);
            return;
        }

        auto threadPool = mgr.GetSystem<ThreadPoolSystem>();
        if (!threadPool) {
            std::cerr << "[VideoSystem] ThreadPoolSystem not available for loading!" << std::endl;
            NotifyLoadComplete(entity, false);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(m_loadMutex);
            m_loadingStatus[entity] = true;
        }

        std::cout << "[VideoSystem] Starting async load for entity " << entity << " from " << filePath << std::endl;

        auto future = threadPool->getIOPool().submit([filePath, entity]() -> LoadResult {
            return LoadVideoInBackground(filePath, entity);
            });

        std::lock_guard<std::mutex> lock(m_loadMutex);
        LoadingTask task;
        task.entityID = entity;
        task.filePath = filePath;
        task.future = std::move(future);
        m_pendingLoads.push_back(std::move(task));
    }

    VideoSystem::LoadResult VideoSystem::LoadVideoInBackground(const std::string& filePath, EntityID entity) {
        LoadResult result;
        result.filePath = filePath;
        result.entityID = entity;
        size_t lastSlash = filePath.find_last_of("/\\");
        result.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "[VideoSystem] Async load: could not open " << filePath << std::endl;
            return result;
        }
        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: no stream info" << std::endl;
            return result;
        }

        int videoStream = -1;
        int audioStream = -1;
        for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream == -1)
                videoStream = i;
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStream == -1)
                audioStream = i;
        }

        if (videoStream == -1) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: no video stream" << std::endl;
            return result;
        }

        AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
        if (!codec) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: decoder not found" << std::endl;
            return result;
        }

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: could not allocate codec context" << std::endl;
            return result;
        }
        if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: failed to set codec parameters" << std::endl;
            return result;
        }
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: could not open codec" << std::endl;
            return result;
        }

        result.width = codecPar->width;
        result.height = codecPar->height;
        result.videoStreamIndex = videoStream;
        result.audioStreamIndex = audioStream;

        AVStream* stream = fmtCtx->streams[videoStream];
        double fps = av_q2d(stream->avg_frame_rate);
        if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
        if (fps <= 0) fps = 30.0;
        result.fps = fps;

        double duration = (fmtCtx->duration != AV_NOPTS_VALUE) ? (double)fmtCtx->duration / AV_TIME_BASE : 0.0;
        result.frameCount = (long long)(duration * fps);
        if (result.frameCount <= 0) result.frameCount = 1000;

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        if (!frame || !pkt) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Async load: could not allocate frame/packet" << std::endl;
            return result;
        }

        SwsContext* swsCtx = sws_getContext(result.width, result.height, codecCtx->pix_fmt,
            result.width, result.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
        avcodec_flush_buffers(codecCtx);

        bool gotFrame = false;
        while (av_read_frame(fmtCtx, pkt) >= 0) {
            if (pkt->stream_index == videoStream) {
                if (avcodec_send_packet(codecCtx, pkt) == 0) {
                    while (avcodec_receive_frame(codecCtx, frame) == 0) {
                        int w = frame->width;
                        int h = frame->height;
                        result.firstFrameRGBA.resize(w * h * 4);
                        uint8_t* dst[1] = { result.firstFrameRGBA.data() };
                        int dstLinesize[1] = { w * 4 };
                        sws_scale(swsCtx, frame->data, frame->linesize, 0, h, dst, dstLinesize);
                        gotFrame = true;
                        break;
                    }
                }
                if (gotFrame) break;
            }
            av_packet_unref(pkt);
        }

        if (!gotFrame) {
            std::cerr << "[VideoSystem] Async load: failed to decode first frame" << std::endl;
        }

        if (audioStream != -1) {
            AVCodecParameters* aCodecPar = fmtCtx->streams[audioStream]->codecpar;
            const AVCodec* aCodec = avcodec_find_decoder(aCodecPar->codec_id);
            if (aCodec) {
                AVCodecContext* aCodecCtx = avcodec_alloc_context3(aCodec);
                if (aCodecCtx && avcodec_parameters_to_context(aCodecCtx, aCodecPar) == 0 &&
                    avcodec_open2(aCodecCtx, aCodec, nullptr) == 0) {
                    int targetRate = 44100;
                    int targetChannels = 2;
                    SwrContext* swr = swr_alloc();
                    if (swr) {
                        uint64_t srcLayout = aCodecCtx->channel_layout;
                        if (srcLayout == 0 && aCodecCtx->channels > 0)
                            srcLayout = av_get_default_channel_layout(aCodecCtx->channels);
                        av_opt_set_int(swr, "in_channel_layout", srcLayout, 0);
                        av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
                        av_opt_set_int(swr, "in_sample_rate", aCodecCtx->sample_rate, 0);
                        av_opt_set_int(swr, "out_sample_rate", targetRate, 0);
                        av_opt_set_sample_fmt(swr, "in_sample_fmt", aCodecCtx->sample_fmt, 0);
                        av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                        if (swr_init(swr) == 0) {
                            avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
                            avcodec_flush_buffers(aCodecCtx);
                            std::vector<float> allPcm;
                            AVPacket* aPkt = av_packet_alloc();
                            AVFrame* aFrame = av_frame_alloc();
                            if (aPkt && aFrame) {
                                while (av_read_frame(fmtCtx, aPkt) >= 0) {
                                    if (aPkt->stream_index != audioStream) {
                                        av_packet_unref(aPkt);
                                        continue;
                                    }
                                    if (avcodec_send_packet(aCodecCtx, aPkt) == 0) {
                                        while (avcodec_receive_frame(aCodecCtx, aFrame) == 0) {
                                            int numSamples = aFrame->nb_samples;
                                            if (numSamples > 0) {
                                                int maxOut = numSamples * 2;
                                                uint8_t* outBuf = (uint8_t*)av_malloc(maxOut * targetChannels * sizeof(float));
                                                if (outBuf) {
                                                    int conv = swr_convert(swr, &outBuf, maxOut,
                                                        (const uint8_t**)aFrame->data, aFrame->nb_samples);
                                                    if (conv > 0) {
                                                        float* fdata = (float*)outBuf;
                                                        size_t oldSize = allPcm.size();
                                                        allPcm.resize(oldSize + conv * targetChannels);
                                                        std::memcpy(allPcm.data() + oldSize, fdata,
                                                            conv * targetChannels * sizeof(float));
                                                    }
                                                    av_free(outBuf);
                                                }
                                            }
                                        }
                                    }
                                    av_packet_unref(aPkt);
                                }
                                int maxOut = 8192;
                                uint8_t* flushBuf = (uint8_t*)av_malloc(maxOut * targetChannels * sizeof(float));
                                if (flushBuf) {
                                    int conv = swr_convert(swr, &flushBuf, maxOut, nullptr, 0);
                                    if (conv > 0) {
                                        float* fdata = (float*)flushBuf;
                                        size_t oldSize = allPcm.size();
                                        allPcm.resize(oldSize + conv * targetChannels);
                                        std::memcpy(allPcm.data() + oldSize, fdata,
                                            conv * targetChannels * sizeof(float));
                                    }
                                    av_free(flushBuf);
                                }
                                if (!allPcm.empty()) {
                                    result.hasAudio = true;
                                    result.audioPcmData = std::move(allPcm);
                                    result.audioChannels = targetChannels;
                                    result.audioSampleRate = targetRate;
                                    result.audioDuration = static_cast<double>(result.audioPcmData.size() / targetChannels) / targetRate;
                                }
                                av_packet_free(&aPkt);
                                av_frame_free(&aFrame);
                            }
                        }
                        swr_free(&swr);
                    }
                    avcodec_free_context(&aCodecCtx);
                }
            }
        }

        result.fmtCtx = fmtCtx;
        result.codecCtx = codecCtx;
        result.swsCtx = swsCtx;
        result.frame = frame;
        result.pkt = pkt;

        try {
            result.fileSize = std::filesystem::file_size(filePath);
            auto ftime = std::filesystem::last_write_time(filePath);
            auto now = std::chrono::system_clock::now();
            auto diff = ftime - std::filesystem::file_time_type::clock::now();
            auto sys_time = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
            std::time_t tt = std::chrono::system_clock::to_time_t(sys_time);
            std::tm tm = *std::localtime(&tt);
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
            result.fileDate = buf;
            strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            result.fileTime = buf;
        }
        catch (...) {}

        result.success = true;
        std::cout << "[VideoSystem] Async load completed for " << filePath << std::endl;
        return result;
    }

    void VideoSystem::ProcessCompletedLoads() {
        std::lock_guard<std::mutex> lock(m_loadMutex);

        for (auto it = m_pendingLoads.begin(); it != m_pendingLoads.end();) {
            if (it->future.valid() &&
                it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                try {
                    LoadResult result = it->future.get();
                    if (result.success) {
                        ApplyLoadedVideo(std::move(result));
                    }
                    else {
                        NotifyLoadComplete(result.entityID, false);
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "[VideoSystem] Load exception: " << e.what() << std::endl;
                    NotifyLoadComplete(it->entityID, false);
                }
                it = m_pendingLoads.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void VideoSystem::ApplyLoadedVideo(LoadResult&& result) {
        EntityID entity = result.entityID;
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            std::cerr << "[VideoSystem] Entity " << entity << " no longer exists, discarding load" << std::endl;
            if (result.fmtCtx) avformat_close_input(&result.fmtCtx);
            if (result.codecCtx) avcodec_free_context(&result.codecCtx);
            if (result.swsCtx) sws_freeContext(result.swsCtx);
            if (result.frame) av_frame_free(&result.frame);
            if (result.pkt) av_packet_free(&result.pkt);
            NotifyLoadComplete(entity, false);
            return;
        }

        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        videoComp.UnloadVideo();

        videoComp.fmtCtx = result.fmtCtx;
        videoComp.codecCtx = result.codecCtx;
        videoComp.swsCtx = result.swsCtx;
        videoComp.frame = result.frame;
        videoComp.pkt = result.pkt;
        videoComp.videoStreamIndex = result.videoStreamIndex;
        videoComp.width = result.width;
        videoComp.height = result.height;
        videoComp.fps = result.fps;
        videoComp.frameCount = result.frameCount;
        videoComp.currentFrame = 0;
        videoComp.isPlaying = false;
        videoComp.playbackSpeed = 1.0f;
        videoComp.frameAccumulator = 0.0f;
        videoComp.filePath = result.filePath;
        videoComp.fileName = result.fileName;
        videoComp.fileSize = result.fileSize;
        videoComp.fileDate = result.fileDate;
        videoComp.fileTime = result.fileTime;
        videoComp.hasExifData = result.hasExif;
        videoComp.hasLSBData = result.hasLSB;
        videoComp.hasAniStudioMetadata = result.hasAniStudio;

        if (!result.firstFrameRGBA.empty()) {
            videoComp.frameDataRGBA = std::move(result.firstFrameRGBA);
            if (m_textureCallback) {
                m_textureCallback(videoComp.GetID(), videoComp.frameDataRGBA.data(),
                    videoComp.width, videoComp.height, 4,
                    &videoComp.currentTexture);
            }
        }

        if (result.hasAudio && !result.audioPcmData.empty()) {
            EntityID audioEntity = mgr.AddNewEntity();
            mgr.AddComponent<AudioComponent>(audioEntity);
            auto& audioComp = mgr.GetComponent<AudioComponent>(audioEntity);
            audioComp.pcmData = std::move(result.audioPcmData);
            audioComp.channels = result.audioChannels;
            audioComp.sampleRate = result.audioSampleRate;
            audioComp.totalSamples = audioComp.pcmData.size() / audioComp.channels;
            audioComp.duration = result.audioDuration;
            audioComp.fileName = std::string("Audio: ") + videoComp.fileName;
            audioComp.filePath = videoComp.filePath;

            mgr.AddComponent<VideoAudioComponent>(entity);
            auto& videoAudioComp = mgr.GetComponent<VideoAudioComponent>(entity);
            videoAudioComp.videoEntityID = entity;
            videoAudioComp.audioEntityID = audioEntity;
            videoAudioComp.hasAudio = true;
            videoAudioComp.volume = 1.0f;
            videoAudioComp.audioEnabled = true;

            for (const auto& cb : videoAudioCallbacks) {
                cb(entity, audioEntity);
            }
        }

        NotifyVideoAdded(entity);
        NotifyLoadComplete(entity, true);
        std::cout << "[VideoSystem] Applied loaded video for entity " << entity << std::endl;
    }

    bool VideoSystem::DecodeNextFrame(VideoComponent& videoComp) {
        if (!videoComp.fmtCtx || !videoComp.codecCtx || videoComp.videoStreamIndex < 0)
            return false;

        AVPacket* pkt = videoComp.pkt;
        AVFrame* frame = videoComp.frame;

        while (av_read_frame(videoComp.fmtCtx, pkt) >= 0) {
            if (pkt->stream_index == videoComp.videoStreamIndex) {
                if (avcodec_send_packet(videoComp.codecCtx, pkt) == 0) {
                    while (avcodec_receive_frame(videoComp.codecCtx, frame) == 0) {
                        int width = frame->width;
                        int height = frame->height;
                        if (width != videoComp.width || height != videoComp.height) {
                            videoComp.width = width;
                            videoComp.height = height;
                            if (videoComp.swsCtx) {
                                sws_freeContext(videoComp.swsCtx);
                            }
                            videoComp.swsCtx = sws_getContext(width, height, videoComp.codecCtx->pix_fmt,
                                width, height, AV_PIX_FMT_RGBA,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);
                        }

                        size_t dataSize = width * height * 4;
                        videoComp.frameDataRGBA.resize(dataSize);

                        uint8_t* dst[1] = { videoComp.frameDataRGBA.data() };
                        int dstLinesize[1] = { width * 4 };
                        sws_scale(videoComp.swsCtx, frame->data, frame->linesize, 0, height, dst, dstLinesize);

                        videoComp.width = width;
                        videoComp.height = height;
                        av_packet_unref(pkt);
                        videoComp.currentFrame++;
                        return true;
                    }
                }
            }
            av_packet_unref(pkt);
        }

        if (videoComp.looping) {
            avformat_seek_file(videoComp.fmtCtx, -1, INT64_MIN, 0, 0, 0);
            avcodec_flush_buffers(videoComp.codecCtx);
            videoComp.currentFrame = 0;
            videoComp.frameAccumulator = 0.0f;
            return DecodeNextFrame(videoComp);
        }
        else {
            videoComp.isPlaying = false;
            videoComp.frameAccumulator = 0.0f;
            return false;
        }
    }

    bool VideoSystem::SaveVideoInBackground(const SaveTaskData& taskData) {
        std::cout << "[VideoSystem] Background save started for: " << taskData.inputPath << std::endl;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, taskData.inputPath.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "[VideoSystem] Failed to open input file: " << taskData.inputPath << std::endl;
            return false;
        }
        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Failed to find stream info" << std::endl;
            return false;
        }

        int videoStream = -1;
        for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStream = i;
                break;
            }
        }

        if (videoStream == -1) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] No video stream found" << std::endl;
            return false;
        }

        AVCodecParameters* vCodecPar = fmtCtx->streams[videoStream]->codecpar;
        const AVCodec* vCodec = avcodec_find_decoder(vCodecPar->codec_id);
        if (!vCodec) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Video decoder not found" << std::endl;
            return false;
        }
        AVCodecContext* vCodecCtx = avcodec_alloc_context3(vCodec);
        if (!vCodecCtx) {
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Could not allocate video codec context" << std::endl;
            return false;
        }
        if (avcodec_parameters_to_context(vCodecCtx, vCodecPar) < 0) {
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Failed to set video codec parameters" << std::endl;
            return false;
        }
        if (avcodec_open2(vCodecCtx, vCodec, nullptr) < 0) {
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Could not open video codec" << std::endl;
            return false;
        }

        avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
        avcodec_flush_buffers(vCodecCtx);

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        if (!frame || !pkt) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Failed to allocate frame/packet" << std::endl;
            return false;
        }

        SwsContext* swsCtx = sws_getContext(
            vCodecPar->width, vCodecPar->height, vCodecCtx->pix_fmt,
            vCodecPar->width, vCodecPar->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsCtx) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
            std::cerr << "[VideoSystem] Failed to create sws context" << std::endl;
            return false;
        }

        std::vector<Utils::VideoFrame> frames;
        frames.reserve(taskData.frameCount > 0 ? static_cast<size_t>(taskData.frameCount) : 1600);
        int decodedFrames = 0;

        while (av_read_frame(fmtCtx, pkt) >= 0) {
            if (pkt->stream_index != videoStream) {
                av_packet_unref(pkt);
                continue;
            }
            if (avcodec_send_packet(vCodecCtx, pkt) == 0) {
                while (avcodec_receive_frame(vCodecCtx, frame) == 0) {
                    int width = frame->width;
                    int height = frame->height;
                    size_t dataSize = width * height * 4;
                    unsigned char* rgbaData = (unsigned char*)malloc(dataSize);
                    if (!rgbaData) {
                        av_packet_unref(pkt);
                        continue;
                    }
                    uint8_t* dst[1] = { rgbaData };
                    int dstLinesize[1] = { width * 4 };
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, height, dst, dstLinesize);
                    Utils::VideoFrame vf;
                    vf.width = width;
                    vf.height = height;
                    vf.channels = 4;
                    vf.data = rgbaData;
                    frames.push_back(vf);
                    decodedFrames++;
                    if (decodedFrames % 100 == 0)
                        std::cout << "[VideoSystem] Decoded " << decodedFrames << " frames" << std::endl;
                }
            }
            av_packet_unref(pkt);
        }

        av_packet_free(&pkt);
        av_frame_free(&frame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&vCodecCtx);
        avformat_close_input(&fmtCtx);

        if (frames.empty()) {
            std::cerr << "[VideoSystem] No frames decoded" << std::endl;
            return false;
        }
        std::cout << "[VideoSystem] Decoded " << frames.size() << " frames" << std::endl;

        // Use copied audio data from taskData, not re-decoded
        Utils::AudioData audioData;
        bool hasAudio = false;
        if (taskData.hasAudio && !taskData.audioPcmData.empty()) {
            audioData.pcmData = taskData.audioPcmData;
            audioData.channels = taskData.audioChannels;
            audioData.sampleRate = taskData.audioSampleRate;
            audioData.duration = taskData.audioDuration;
            hasAudio = true;
            std::cout << "[VideoSystem] Using copied audio data: " << audioData.pcmData.size() << " samples" << std::endl;
        }

        nlohmann::json metadata;
        metadata["fps"] = taskData.fps;
        metadata["width"] = taskData.width;
        metadata["height"] = taskData.height;
        metadata["frameCount"] = frames.size();
        metadata["originalFile"] = taskData.inputPath;
        metadata["hasAudio"] = hasAudio;
        if (hasAudio) {
            metadata["audioChannels"] = audioData.channels;
            metadata["audioSampleRate"] = audioData.sampleRate;
            metadata["audioDuration"] = audioData.duration;
        }

        bool result = Utils::VideoUtils::EncodeFramesToVideo(
            frames,
            taskData.outputPath,
            taskData.fps,
            metadata,
            hasAudio ? &audioData : nullptr
        );

        for (auto& f : frames) {
            if (f.data) free((void*)f.data);
        }

        if (result) {
            std::cout << "[VideoSystem] Successfully saved video to: " << taskData.outputPath << std::endl;
        }
        else {
            std::cerr << "[VideoSystem] Failed to save video" << std::endl;
        }
        return result;
    }

    void VideoSystem::ProcessCompletedSaves() {
        std::lock_guard<std::mutex> lock(m_saveMutex);

        for (auto it = m_saveFutures.begin(); it != m_saveFutures.end();) {
            if (it->second.valid() &&
                it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                try {
                    bool success = it->second.get();
                    EntityID entity = it->first;
                    std::string path = m_savePaths[entity];
                    if (success) {
                        if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity)) {
                            auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
                            videoComp.filePath = path;
                            videoComp.fileName = std::filesystem::path(path).filename().string();
                        }
                    }
                    NotifySaveComplete(entity, success, path);
                }
                catch (const std::exception& e) {
                    std::cerr << "[VideoSystem] Save exception: " << e.what() << std::endl;
                    NotifySaveComplete(it->first, false, "");
                }
                it = m_saveFutures.erase(it);
                m_savePaths.erase(it->first);
            }
            else {
                ++it;
            }
        }
    }

    void VideoSystem::NotifyVideoAdded(EntityID entity) {
        for (const auto& cb : videoAddedCallbacks) {
            try { cb(entity); }
            catch (...) {}
        }
    }

    void VideoSystem::NotifyVideoRemoved(EntityID entity) {
        for (const auto& cb : videoRemovedCallbacks) {
            try { cb(entity); }
            catch (...) {}
        }
    }

    void VideoSystem::NotifySaveComplete(EntityID entity, bool success, const std::string& path) {
        for (const auto& cb : saveCallbacks) {
            try { cb(entity, success, path); }
            catch (...) {}
        }
        std::cout << "[VideoSystem] Save " << (success ? "completed" : "failed")
            << " for entity " << entity << std::endl;
    }

    void VideoSystem::NotifyLoadComplete(EntityID entity, bool success) {
        std::lock_guard<std::mutex> lock(m_loadMutex);
        m_loadingStatus[entity] = false;
        for (const auto& cb : loadCallbacks) {
            try { cb(entity, success); }
            catch (...) {}
        }
        std::cout << "[VideoSystem] Load " << (success ? "completed" : "failed")
            << " for entity " << entity << std::endl;
    }

}