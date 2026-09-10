#include "VideoSystem.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <algorithm>

namespace ECS {

    VideoSystem::VideoSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "VideoSystem";
        AddComponentSignature<VideoComponent>();
    }

    VideoSystem::~VideoSystem() {
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

    VideoSystem::LoadResult VideoSystem::LoadVideoInBackground(const std::string& filePath, EntityID entity) {
        LoadResult result;
        result.filePath = filePath;
        result.entityID = entity;
        size_t lastSlash = filePath.find_last_of("/\\");
        result.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, filePath.c_str(), nullptr, nullptr) < 0) {
            return result;
        }
        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            avformat_close_input(&fmtCtx);
            return result;
        }

        int videoStream = -1;
        for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
            if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream == -1)
                videoStream = i;
        }

        if (videoStream == -1) {
            avformat_close_input(&fmtCtx);
            return result;
        }

        AVCodecParameters* codecPar = fmtCtx->streams[videoStream]->codecpar;
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

        result.width = codecPar->width;
        result.height = codecPar->height;
        result.videoStreamIndex = videoStream;

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
            return result;
        }

        SwsContext* swsCtx = sws_getContext(result.width, result.height, codecCtx->pix_fmt,
            result.width, result.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        avformat_seek_file(fmtCtx, -1, INT64_MIN, 0, 0, 0);
        avcodec_flush_buffers(codecCtx);

        bool gotFrame = false;
        int maxAttempts = 200;
        while (av_read_frame(fmtCtx, pkt) >= 0 && maxAttempts-- > 0) {
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
        av_packet_unref(pkt);

        if (!gotFrame) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            sws_freeContext(swsCtx);
            avcodec_free_context(&codecCtx);
            avformat_close_input(&fmtCtx);
            return result;
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
        return result;
    }

    void VideoSystem::ProcessCompletedLoads() {
        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);

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
            NotifyLoadComplete(entity, false);
            return;
        }

        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);

        videoComp.fmtCtx.reset();
        videoComp.codecCtx.reset();
        videoComp.swsCtx.reset();
        videoComp.frame.reset();
        videoComp.pkt.reset();

        videoComp.fmtCtx.reset(result.fmtCtx);
        videoComp.codecCtx.reset(result.codecCtx);
        videoComp.swsCtx.reset(result.swsCtx);
        videoComp.frame.reset(result.frame);
        videoComp.pkt.reset(result.pkt);
        videoComp.videoStreamIndex = result.videoStreamIndex;
        videoComp.width = result.width;
        videoComp.height = result.height;
        videoComp.fps = result.fps;
        videoComp.frameCount = result.frameCount;
        videoComp.currentFrame = 0;
        videoComp.filePath = result.filePath;
        videoComp.fileName = result.fileName;
        videoComp.fileSize = result.fileSize;
        videoComp.fileDate = result.fileDate;
        videoComp.fileTime = result.fileTime;
        videoComp.hasExifData = result.hasExif;
        videoComp.hasLSBData = result.hasLSB;
        videoComp.hasAniStudioMetadata = result.hasAniStudio;

        result.fmtCtx = nullptr;
        result.codecCtx = nullptr;
        result.swsCtx = nullptr;
        result.frame = nullptr;
        result.pkt = nullptr;

        if (!result.firstFrameRGBA.empty()) {
            videoComp.frameDataRGBA = std::move(result.firstFrameRGBA);
            videoComp.needsTextureUpdate = true;
        }

        NotifyVideoAdded(entity);
        NotifyLoadComplete(entity, true);
    }

    void VideoSystem::SetVideo(EntityID entity, const std::string& filePath) {
        if (mgr.HasComponent<VideoComponent>(entity)) {
            auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
            videoComp.fmtCtx.reset();
            videoComp.codecCtx.reset();
            videoComp.swsCtx.reset();
            videoComp.frame.reset();
            videoComp.pkt.reset();
            videoComp.frameDataRGBA.clear();
            videoComp.width = 0;
            videoComp.height = 0;
            videoComp.fps = 0.0;
            videoComp.frameCount = 0;
            videoComp.currentFrame = 0;
            videoComp.currentTime = 0.0;
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

            NotifyVideoRemoved(entity);

            videoComp.fmtCtx.reset();
            videoComp.codecCtx.reset();
            videoComp.swsCtx.reset();
            videoComp.frame.reset();
            videoComp.pkt.reset();
            videoComp.frameDataRGBA.clear();
            entities.erase(entity);

            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveFutures.erase(entity);
            m_savePaths.erase(entity);
            std::lock_guard<std::recursive_mutex> lock2(m_loadMutex);
            m_loadingStatus.erase(entity);
        }
    }

    void VideoSystem::ClearCache(EntityID entity) {
        {
            std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
            m_loadingStatus.erase(entity);
            m_pendingLoads.erase(
                std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
                    [entity](const LoadingTask& t) { return t.entityID == entity; }),
                m_pendingLoads.end());
        }

        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveFutures.erase(entity);
            m_savePaths.erase(entity);
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity)) {
            auto& vc = mgr.GetComponent<VideoComponent>(entity);
            vc.fmtCtx.reset();
            vc.codecCtx.reset();
            vc.swsCtx.reset();
            vc.frame.reset();
            vc.pkt.reset();
            vc.frameDataRGBA.clear();
            vc.needsTextureUpdate = false;
            vc.width = 0;
            vc.height = 0;
            vc.fps = 0.0;
            vc.frameCount = 0;
            vc.currentFrame = 0;
            vc.currentTime = 0.0;
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

    void VideoSystem::RegisterSaveCallback(const SaveCallback& cb) {
        saveCallbacks.push_back(cb);
    }

    void VideoSystem::RegisterLoadCallback(const LoadCallback& cb) {
        loadCallbacks.push_back(cb);
    }

    void VideoSystem::SetVideoTextureCallback(const VideoTextureCallback& callback) {
        m_textureCallback = callback;
    }

    bool VideoSystem::IsLoading(EntityID entity) const {
        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
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

        std::string savePath = outputPath;
        if (savePath.empty()) {
            std::string baseName = std::filesystem::path(videoComp.filePath).stem().string();
            std::string ext = ".mp4";
            std::string dir = std::filesystem::path(videoComp.filePath).parent_path().string();
            savePath = dir + "/" + baseName + "_saved" + ext;
        }
        taskData.outputPath = savePath;

        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_savePaths[entity] = savePath;
            m_saveFutures[entity] = threadPool->getIOPool().submit([this, taskData]() -> bool {
                return SaveVideoInBackground(taskData);
                });
        }
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
            std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
            m_loadingStatus[entity] = true;
        }

        auto future = threadPool->getIOPool().submit([filePath, entity]() -> LoadResult {
            return LoadVideoInBackground(filePath, entity);
            });

        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
        LoadingTask task;
        task.entityID = entity;
        task.filePath = filePath;
        task.future = std::move(future);
        m_pendingLoads.push_back(std::move(task));
    }

    bool VideoSystem::SaveVideoInBackground(const SaveTaskData& taskData) {
        AVFormatContext* fmtCtx = nullptr;
        if (avformat_open_input(&fmtCtx, taskData.inputPath.c_str(), nullptr, nullptr) < 0) {
            return false;
        }
        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            avformat_close_input(&fmtCtx);
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
            return false;
        }

        AVCodecParameters* vCodecPar = fmtCtx->streams[videoStream]->codecpar;
        const AVCodec* vCodec = avcodec_find_decoder(vCodecPar->codec_id);
        if (!vCodec) {
            avformat_close_input(&fmtCtx);
            return false;
        }
        AVCodecContext* vCodecCtx = avcodec_alloc_context3(vCodec);
        if (!vCodecCtx) {
            avformat_close_input(&fmtCtx);
            return false;
        }
        if (avcodec_parameters_to_context(vCodecCtx, vCodecPar) < 0) {
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
            return false;
        }
        if (avcodec_open2(vCodecCtx, vCodec, nullptr) < 0) {
            avcodec_free_context(&vCodecCtx);
            avformat_close_input(&fmtCtx);
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
                    auto rgbaData = std::make_unique<unsigned char[]>(dataSize);
                    if (!rgbaData) {
                        av_packet_unref(pkt);
                        continue;
                    }
                    uint8_t* dst[1] = { rgbaData.get() };
                    int dstLinesize[1] = { width * 4 };
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, height, dst, dstLinesize);
                    Utils::VideoFrame vf;
                    vf.width = width;
                    vf.height = height;
                    vf.channels = 4;
                    vf.data = rgbaData.release();
                    frames.push_back(vf);
                    decodedFrames++;
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
            return false;
        }

        nlohmann::json metadata;
        metadata["fps"] = taskData.fps;
        metadata["width"] = taskData.width;
        metadata["height"] = taskData.height;
        metadata["frameCount"] = frames.size();
        metadata["originalFile"] = taskData.inputPath;
        metadata["hasAudio"] = false;

        bool result = Utils::VideoUtils::EncodeFramesToVideo(
            frames,
            taskData.outputPath,
            taskData.fps,
            metadata,
            nullptr
        );

        for (auto& f : frames) {
            if (f.data) free((void*)f.data);
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
                auto pathIt = m_savePaths.find(it->first);
                if (pathIt != m_savePaths.end()) {
                    m_savePaths.erase(pathIt);
                }
                it = m_saveFutures.erase(it);
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
    }

    void VideoSystem::NotifyLoadComplete(EntityID entity, bool success) {
        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
        m_loadingStatus[entity] = false;
        for (const auto& cb : loadCallbacks) {
            try { cb(entity, success); }
            catch (...) {}
        }
    }

    bool VideoSystem::DecodeFrameForSave(VideoComponent& videoComp, long long frameIndex, bool& isNewFrame) {
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0) {
            isNewFrame = false;
            return false;
        }

        long long targetFrame = frameIndex;
        if (targetFrame < 0) targetFrame = 0;
        if (targetFrame >= videoComp.frameCount) {
            targetFrame = videoComp.frameCount - 1;
        }

        if (videoComp.currentFrame == targetFrame && !videoComp.frameDataRGBA.empty()) {
            isNewFrame = false;
            return true;
        }

        double timeSec = static_cast<double>(targetFrame) / videoComp.fps;
        int64_t target_ts = static_cast<int64_t>(timeSec * AV_TIME_BASE);

        int ret = avformat_seek_file(videoComp.fmtCtx.get(), videoComp.videoStreamIndex,
            INT64_MIN, target_ts, target_ts, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            isNewFrame = false;
            return false;
        }

        avcodec_flush_buffers(videoComp.codecCtx.get());

        videoComp.currentFrame = 0;

        AVPacket* pkt = videoComp.pkt.get();
        AVFrame* frame = videoComp.frame.get();

        int maxAttempts = 500;
        int decodedFrames = 0;

        while (av_read_frame(videoComp.fmtCtx.get(), pkt) >= 0 && maxAttempts-- > 0) {
            if (pkt->stream_index == videoComp.videoStreamIndex) {
                if (avcodec_send_packet(videoComp.codecCtx.get(), pkt) == 0) {
                    while (avcodec_receive_frame(videoComp.codecCtx.get(), frame) == 0) {
                        decodedFrames++;

                        if (decodedFrames - 1 == targetFrame) {
                            int width = frame->width;
                            int height = frame->height;

                            if (width != videoComp.width || height != videoComp.height) {
                                videoComp.width = width;
                                videoComp.height = height;
                                videoComp.swsCtx.reset();
                                videoComp.swsCtx.reset(sws_getContext(width, height, videoComp.codecCtx->pix_fmt,
                                    width, height, AV_PIX_FMT_RGBA,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr));
                            }

                            size_t dataSize = width * height * 4;
                            videoComp.frameDataRGBA.resize(dataSize);

                            uint8_t* dst[1] = { videoComp.frameDataRGBA.data() };
                            int dstLinesize[1] = { width * 4 };
                            sws_scale(videoComp.swsCtx.get(), frame->data, frame->linesize, 0, height, dst, dstLinesize);

                            videoComp.currentFrame = targetFrame;
                            av_packet_unref(pkt);
                            isNewFrame = true;
                            return true;
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }

        isNewFrame = false;
        return false;
    }

}