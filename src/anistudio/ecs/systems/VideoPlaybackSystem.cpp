#include "VideoPlaybackSystem.hpp"
#include "Log.hpp"

#include <algorithm>
#include <limits>
#include <cmath>
#include <cstdlib>

namespace ECS {

    const std::vector<std::string> VideoPlaybackSystem::s_hwAccelDevices = {
        "cuda", "vaapi", "dxva2", "d3d11va", "vulkan", "videotoolbox"
    };

    void VideoPlaybackSystem::VideoTrack::Clock::Reset(double startTime) {
        m_currentTime = startTime;
        m_lastUpdate = std::chrono::steady_clock::now();
        m_paused = true;
    }

    void VideoPlaybackSystem::VideoTrack::Clock::Play() {
        if (m_paused) {
            m_lastUpdate = std::chrono::steady_clock::now();
            m_paused = false;
        }
    }

    void VideoPlaybackSystem::VideoTrack::Clock::Pause() {
        if (!m_paused) {
            m_currentTime = Now();
            m_paused = true;
        }
    }

    void VideoPlaybackSystem::VideoTrack::Clock::Seek(double time) {
        m_currentTime = time;
        m_lastUpdate = std::chrono::steady_clock::now();
        m_paused = true;
    }

    void VideoPlaybackSystem::VideoTrack::Clock::SetSpeed(float speed) {
        if (!m_paused) {
            m_currentTime = Now();
            m_lastUpdate = std::chrono::steady_clock::now();
        }
        m_speed = speed;
    }

    double VideoPlaybackSystem::VideoTrack::Clock::Now() const {
        if (m_paused) {
            return m_currentTime;
        }
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - m_lastUpdate).count() * m_speed;
        return m_currentTime + elapsed;
    }

    VideoPlaybackSystem::VideoTrack::VideoTrack(VideoPlaybackSystem* owner, EntityManager& mgr, EntityID entity,
        bool hasAudio, double duration, double fps)
        : entity(entity), hasAudio(hasAudio), duration(duration), fps(fps > 0.0 ? fps : 30.0),
        m_owner(owner), m_mgr(mgr) {
        ANI_LOG_DEBUG("[VideoTrack] Created for entity %u", entity);

        const char* hwDev = std::getenv("ANI_VIDEO_HW_ACCEL");
        if (hwDev) {
            std::string dev(hwDev);
            std::transform(dev.begin(), dev.end(), dev.begin(), ::tolower);
            for (const auto& supported : s_hwAccelDevices) {
                if (dev == supported) {
                    hwAccelEnabled = true;
                    ANI_LOG_INFO("[VideoTrack] HW acceleration enabled: %s", dev.c_str());
                    break;
                }
            }
        }
    }

    VideoPlaybackSystem::VideoTrack::~VideoTrack() {
        ANI_LOG_DEBUG("[VideoTrack] Destroying for entity %u", entity);
        if (hwDeviceCtx) {
            av_buffer_unref(&hwDeviceCtx);
            hwDeviceCtx = nullptr;
        }
        m_running.store(false);
        m_cmdCV.notify_all();
        m_bufferCV.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void VideoPlaybackSystem::VideoTrack::StartThread() {
        ANI_LOG_DEBUG("[VideoTrack] Starting thread for entity %u", entity);
        m_thread = std::thread(&VideoTrack::WorkerLoop, this);
    }

    void VideoPlaybackSystem::VideoTrack::PushCommand(WorkerCmd cmd) {
        {
            std::lock_guard<std::mutex> lock(m_cmdMutex);
            m_commands.push_back(cmd);
        }
        m_cmdCV.notify_all();
        m_bufferCV.notify_all();
    }

    void VideoPlaybackSystem::VideoTrack::RequestSeek(double time, bool pauseAfter) {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::Seek;
        cmd.seekTime = time;
        cmd.pauseAfter = pauseAfter;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::RequestPause() {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::Pause;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::RequestResume() {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::Resume;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::RequestStop() {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::Stop;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::RequestShutdown() {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::Shutdown;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::RequestBuildIndex() {
        WorkerCmd cmd;
        cmd.type = WorkerCmdType::BuildIndex;
        PushCommand(cmd);
    }

    void VideoPlaybackSystem::VideoTrack::ClearBuffer() {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffer.clear();
        m_bufferCV.notify_all();
    }

    bool VideoPlaybackSystem::VideoTrack::TryPopDisplayFrame(double upToTime, RingFrame& out) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        bool got = false;
        while (!m_buffer.empty() && m_buffer.front().pts <= upToTime) {
            out = std::move(m_buffer.front());
            m_buffer.pop_front();
            got = true;
        }
        if (got) {
            m_bufferCV.notify_all();
        }
        return got;
    }

    void VideoPlaybackSystem::ClearCache(EntityID entity) {
        std::unique_ptr<VideoTrack> track;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it == m_tracks.end()) {
                ANI_LOG_TRACE("[VideoPlaybackSystem] ClearCache: no track for entity %u", entity);
                return;
            }
            track = std::move(it->second);
            m_tracks.erase(it);
        }
        track.reset();
        ANI_LOG_DEBUG("[VideoPlaybackSystem] Cleared track for entity %u", entity);
    }

    bool VideoPlaybackSystem::VideoTrack::InitHWAccel(AVCodecContext* codecCtx) {
        if (!hwAccelEnabled) return false;
        if (!codecCtx) return false;

        const char* deviceName = nullptr;
        AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;

        for (const auto& dev : s_hwAccelDevices) {
            AVHWDeviceType type = av_hwdevice_find_type_by_name(dev.c_str());
            if (type != AV_HWDEVICE_TYPE_NONE) {
                hwType = type;
                deviceName = dev.c_str();
                break;
            }
        }

        if (hwType == AV_HWDEVICE_TYPE_NONE) {
            ANI_LOG_WARN("[VideoTrack] No compatible HW acceleration found, falling back to CPU");
            hwAccelEnabled = false;
            return false;
        }

        int ret = av_hwdevice_ctx_create(&hwDeviceCtx, hwType, nullptr, nullptr, 0);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_strerror(ret, errbuf, sizeof(errbuf));
            ANI_LOG_WARN("[VideoTrack] Failed to create HW device context: %s", errbuf);
            hwAccelEnabled = false;
            return false;
        }

        codecCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
        if (!codecCtx->hw_device_ctx) {
            ANI_LOG_WARN("[VideoTrack] Failed to ref HW device context");
            hwAccelEnabled = false;
            return false;
        }

        ANI_LOG_INFO("[VideoTrack] HW acceleration initialized: %s", deviceName);
        return true;
    }

    bool VideoPlaybackSystem::VideoTrack::DecodeOneFrame(RingFrame& out) {
        if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
            return false;
        }
        auto& videoComp = m_mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0 || !videoComp.pkt || !videoComp.frame) {
            return false;
        }

        AVPacket* pkt = videoComp.pkt.get();
        AVFrame* frame = videoComp.frame.get();
        AVStream* stream = videoComp.fmtCtx->streams[videoComp.videoStreamIndex];

        int maxAttempts = 200;
        while (maxAttempts-- > 0) {
            if (av_read_frame(videoComp.fmtCtx.get(), pkt) < 0) {
                return false;
            }
            if (pkt->stream_index != videoComp.videoStreamIndex) {
                av_packet_unref(pkt);
                continue;
            }

            bool produced = false;
            if (avcodec_send_packet(videoComp.codecCtx.get(), pkt) == 0) {
                while (avcodec_receive_frame(videoComp.codecCtx.get(), frame) == 0) {
                    AVFrame* swFrame = frame;
                    AVFrame* tempFrame = nullptr;

                    if (hwAccelEnabled && frame->hw_frames_ctx) {
                        tempFrame = av_frame_alloc();
                        if (!tempFrame) continue;
                        int ret = av_hwframe_transfer_data(tempFrame, frame, 0);
                        if (ret < 0) {
                            av_frame_free(&tempFrame);
                            continue;
                        }
                        tempFrame->pts = frame->pts;
                        tempFrame->best_effort_timestamp = frame->best_effort_timestamp;
                        swFrame = tempFrame;
                    }

                    int w = swFrame->width;
                    int h = swFrame->height;
                    if (w <= 0 || h <= 0) {
                        if (tempFrame) av_frame_free(&tempFrame);
                        continue;
                    }

                    if (w != videoComp.width || h != videoComp.height || !videoComp.swsCtx) {
                        videoComp.width = w;
                        videoComp.height = h;
                        videoComp.swsCtx.reset(sws_getContext(w, h, videoComp.codecCtx->pix_fmt,
                            w, h, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr));
                    }

                    int64_t ts = (swFrame->best_effort_timestamp != AV_NOPTS_VALUE)
                        ? swFrame->best_effort_timestamp : swFrame->pts;

                    double pts;
                    if (ts != AV_NOPTS_VALUE) {
                        pts = static_cast<double>(ts) * av_q2d(stream->time_base);
                    }
                    else {
                        pts = static_cast<double>(videoComp.currentFrame + 1) / fps;
                    }

                    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
                    out.data.resize(size);
                    uint8_t* dst[1] = { out.data.data() };
                    int linesize[1] = { w * 4 };
                    sws_scale(videoComp.swsCtx.get(), swFrame->data, swFrame->linesize, 0, h, dst, linesize);

                    out.width = w;
                    out.height = h;
                    out.pts = pts;
                    out.frameIndex = static_cast<long long>(pts * fps + 0.5);

                    if (tempFrame) {
                        av_frame_free(&tempFrame);
                    }

                    produced = true;
                    break;
                }
            }

            av_packet_unref(pkt);
            if (produced) {
                return true;
            }
        }
        return false;
    }

    void VideoPlaybackSystem::VideoTrack::BuildFrameIndex() {
        if (indexReady || m_indexing.load()) return;
        m_indexing.store(true);

        if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
            ANI_LOG_WARN("[BuildFrameIndex] Invalid entity %u", entity);
            m_indexing.store(false);
            return;
        }
        auto& videoComp = m_mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0) {
            ANI_LOG_WARN("[BuildFrameIndex] No fmtCtx/video stream for entity %u", entity);
            m_indexing.store(false);
            return;
        }

        AVFormatContext* fmtCtx = videoComp.fmtCtx.get();
        int streamIndex = videoComp.videoStreamIndex;
        AVStream* stream = fmtCtx->streams[streamIndex];
        streamTimeBase = stream->time_base;

        ANI_LOG_DEBUG("[BuildFrameIndex] Building index for entity %u", entity);

        av_seek_frame(fmtCtx, streamIndex, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(videoComp.codecCtx.get());

        AVPacket* pkt = av_packet_alloc();
        if (!pkt) {
            ANI_LOG_WARN("[BuildFrameIndex] av_packet_alloc failed for entity %u", entity);
            m_indexing.store(false);
            return;
        }

        int64_t frameNum = 0;

        while (av_read_frame(fmtCtx, pkt) >= 0) {
            if (pkt->stream_index != streamIndex) {
                av_packet_unref(pkt);
                continue;
            }

            bool isKeyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;

            FrameIndexEntry entry;
            entry.frameIndex = frameNum;
            entry.packetPos = pkt->pos;
            entry.keyframePos = pkt->pos;
            entry.pts = pkt->pts;
            entry.isKeyframe = isKeyframe;
            frameIndex.push_back(entry);

            frameNum++;
            av_packet_unref(pkt);
        }

        av_packet_free(&pkt);
        indexReady = true;
        m_indexing.store(false);
        ANI_LOG_DEBUG("[BuildFrameIndex] Index built: %zu frames for entity %u",
            frameIndex.size(), entity);
    }

    bool VideoPlaybackSystem::VideoTrack::DecodeFrameAt(int64_t targetPts, RingFrame& out) {
        if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
            return false;
        }
        auto& videoComp = m_mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.videoStreamIndex < 0 || !videoComp.pkt || !videoComp.frame) {
            return false;
        }

        if (!indexReady) {
            return false;
        }

        size_t idx = 0;
        for (size_t i = 0; i < frameIndex.size(); ++i) {
            if (frameIndex[i].pts <= targetPts) {
                idx = i;
            }
            else {
                break;
            }
        }

        const FrameIndexEntry& entry = frameIndex[idx];
        int64_t keyframePts = entry.pts;

        int ret = avformat_seek_file(videoComp.fmtCtx.get(), videoComp.videoStreamIndex,
            INT64_MIN, keyframePts, INT64_MAX, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            ret = av_seek_frame(videoComp.fmtCtx.get(), videoComp.videoStreamIndex,
                keyframePts, AVSEEK_FLAG_BACKWARD);
            if (ret < 0) {
                ANI_LOG_WARN("[DecodeFrameAt] Seek failed for entity %u (target pts %lld)",
                    entity, static_cast<long long>(targetPts));
                return false;
            }
        }

        avcodec_flush_buffers(videoComp.codecCtx.get());

        RingFrame temp;
        int maxAttempts = 10000;
        double targetPtsSeconds = static_cast<double>(targetPts) * av_q2d(streamTimeBase);
        double tolerance = 0.001;
        while (maxAttempts-- > 0) {
            if (!DecodeOneFrame(temp)) {
                break;
            }
            if (temp.pts >= targetPtsSeconds - tolerance) {
                out = std::move(temp);
                return true;
            }
        }

        ANI_LOG_WARN("[DecodeFrameAt] Failed to reach target pts %lld for entity %u",
            static_cast<long long>(targetPts), entity);
        return false;
    }

    void VideoPlaybackSystem::VideoTrack::PerformSeek(double time) {
        if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
            ANI_LOG_WARN("[PerformSeek] Invalid entity %u", entity);
            return;
        }
        auto& videoComp = m_mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.frameCount <= 0 || videoComp.videoStreamIndex < 0) {
            ANI_LOG_WARN("[PerformSeek] No loaded video for entity %u", entity);
            return;
        }

        if (!indexReady && !m_indexing.load()) {
            pendingSeekTimeStore = time;
            RequestBuildIndex();
            ANI_LOG_DEBUG("[PerformSeek] Index not ready for entity %u, requested build (pending seek to %.3f)",
                entity, time);
            return;
        }

        if (m_indexing.load()) {
            pendingSeekTimeStore = time;
            ANI_LOG_TRACE("[PerformSeek] Indexing in progress for entity %u, deferred seek to %.3f",
                entity, time);
            return;
        }

        double dur = videoComp.frameCount / fps;
        time = std::clamp(time, 0.0, dur > 0.0 ? dur - 0.001 : 0.0);

        int64_t targetPts = static_cast<int64_t>(time / av_q2d(streamTimeBase));

        RingFrame frame;
        if (DecodeFrameAt(targetPts, frame)) {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            m_buffer.clear();
            m_buffer.push_back(std::move(frame));
            m_bufferCV.notify_all();
            ANI_LOG_DEBUG("[PerformSeek] Seeked entity %u to %.3fs", entity, time);
        }
        else {
            ANI_LOG_WARN("[PerformSeek] Decode at target failed for entity %u, resetting flags", entity);
            if (m_owner) {
                std::lock_guard<std::mutex> lock(m_owner->m_tracksMutex);
                auto it = m_owner->m_tracks.find(entity);
                if (it != m_owner->m_tracks.end()) {
                    it->second->seekPending = false;
                    it->second->seeking = false;
                }
            }
        }
    }

    void VideoPlaybackSystem::VideoTrack::WorkerLoop() {
        ANI_LOG_DEBUG("[WorkerLoop] Started for entity %u", entity);

        if (hwAccelEnabled) {
            if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
                hwAccelEnabled = false;
            }
            else {
                auto& videoComp = m_mgr.GetComponent<VideoComponent>(entity);
                if (videoComp.codecCtx) {
                    InitHWAccel(videoComp.codecCtx.get());
                }
            }
        }

        while (m_running.load()) {
            if (!m_mgr.IsEntityValid(entity) || !m_mgr.HasComponent<VideoComponent>(entity)) {
                ANI_LOG_DEBUG("[WorkerLoop] Entity %u invalid, exiting", entity);
                m_running.store(false);
                break;
            }

            std::vector<WorkerCmd> cmds;
            {
                std::lock_guard<std::mutex> lock(m_cmdMutex);
                while (!m_commands.empty()) {
                    cmds.push_back(m_commands.front());
                    m_commands.pop_front();
                }
            }

            for (auto& cmd : cmds) {
                if (!m_running.load()) break;
                switch (cmd.type) {
                case WorkerCmdType::BuildIndex: {
                    BuildFrameIndex();
                    if (pendingSeekTimeStore >= 0.0) {
                        double t = pendingSeekTimeStore;
                        pendingSeekTimeStore = -1.0;
                        RequestSeek(t, true);
                    }
                    break;
                }
                case WorkerCmdType::Seek: {
                    ClearBuffer();
                    PerformSeek(cmd.seekTime);
                    m_workerPaused.store(cmd.pauseAfter);
                    break;
                }
                case WorkerCmdType::Pause:
                    m_workerPaused.store(true);
                    break;
                case WorkerCmdType::Resume:
                    m_workerPaused.store(false);
                    break;
                case WorkerCmdType::Stop:
                    m_workerPaused.store(true);
                    ClearBuffer();
                    break;
                case WorkerCmdType::Shutdown:
                    m_running.store(false);
                    break;
                }
            }

            if (!m_running.load()) break;

            if (m_workerPaused.load()) {
                std::unique_lock<std::mutex> lock(m_cmdMutex);
                m_cmdCV.wait_for(lock, std::chrono::milliseconds(50), [&] {
                    return !m_commands.empty() || !m_running.load();
                    });
                continue;
            }

            if (!indexReady && !m_indexing.load()) {
                RequestBuildIndex();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (m_indexing.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            {
                std::unique_lock<std::mutex> lock(m_bufferMutex);
                m_bufferCV.wait_for(lock, std::chrono::milliseconds(50), [&] {
                    return m_buffer.size() < kBufferCapacity || !m_running.load();
                    });
            }
            if (!m_running.load()) break;

            {
                std::lock_guard<std::mutex> lock(m_cmdMutex);
                if (!m_commands.empty()) continue;
            }

            RingFrame frame;
            if (DecodeOneFrame(frame)) {
                std::lock_guard<std::mutex> lock(m_bufferMutex);
                if (m_buffer.size() < kBufferCapacity) {
                    m_buffer.push_back(std::move(frame));
                    m_bufferCV.notify_all();
                }
            }
            else {
                m_workerPaused.store(true);
                if (m_owner) {
                    std::lock_guard<std::mutex> lock(m_owner->m_tracksMutex);
                    auto it = m_owner->m_tracks.find(entity);
                    if (it != m_owner->m_tracks.end()) {
                        m_owner->HandleEndOfStream(entity, *it->second);
                    }
                }
            }
        }
        ANI_LOG_DEBUG("[WorkerLoop] Exited for entity %u", entity);
    }

    VideoPlaybackSystem::VideoPlaybackSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "VideoPlaybackSystem";
        AddComponentSignature<VideoComponent>();
    }

    VideoPlaybackSystem::~VideoPlaybackSystem() {
        Destroy();
    }

    void VideoPlaybackSystem::Start() {
        m_audioPlayback = mgr.GetSystem<AudioPlaybackSystem>().get();
        if (!m_audioPlayback) {
            ANI_LOG_WARN("[VideoPlaybackSystem] AudioPlaybackSystem not available; video will play without audio sync");
        }
        else {
            ANI_LOG_INFO("[VideoPlaybackSystem] Started");
        }
    }

    void VideoPlaybackSystem::Update(float deltaT) {
        if (m_destroying.load()) return;

        std::vector<EntityID> endedEntities;

        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            for (auto& pair : m_tracks) {
                EntityID entity = pair.first;
                VideoTrack& track = *pair.second;

                if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
                    continue;
                }

                bool isPaused = track.paused || track.reachedEnd || track.stopped;

                if (track.restartPending) {
                    track.restartPending = false;
                    track.reachedEnd = false;
                    track.paused = false;
                    track.stopped = false;
                    track.clock.Seek(0.0);
                    track.clock.Play();
                    track.RequestSeek(0.0, false);
                    if (track.hasAudio && m_audioPlayback) {
                        m_audioPlayback->Seek(entity, 0.0);
                        m_audioPlayback->Play(entity, true);
                    }
                    continue;
                }

                double masterClockTime;

                if (track.hasAudio && m_audioPlayback) {
                    masterClockTime = m_audioPlayback->GetCurrentPosition(entity);
                }
                else {
                    masterClockTime = track.clock.Now();
                }

                if (track.seekPending) {
                    masterClockTime = track.pendingSeekTime;
                }

                if (!isPaused && track.duration > 0.0 && masterClockTime >= track.duration) {
                    endedEntities.push_back(entity);
                    continue;
                }

                RingFrame frame;
                bool got = (isPaused || track.seeking || track.seekPending)
                    ? track.TryPopDisplayFrame(std::numeric_limits<double>::max(), frame)
                    : track.TryPopDisplayFrame(masterClockTime, frame);

                if (got) {
                    auto& videoComp = mgr.GetComponent<VideoComponent>(entity);

                    if (track.firstFrameAfterSeek) {
                        track.firstFrameAfterSeek = false;
                        track.lastDisplayedPts = frame.pts;
                    }
                    else if (frame.pts < track.lastDisplayedPts - 0.5) {
                        continue;
                    }

                    videoComp.UpdateFrameData(std::move(frame.data), frame.width, frame.height, frame.frameIndex);
                    track.lastDisplayedPts = frame.pts;

                    if (track.seekPending) {
                        track.seekPending = false;
                        track.seeking = false;

                        if (track.wasPlayingBeforeSeek) {
                            track.wasPlayingBeforeSeek = false;
                            track.paused = false;
                            track.clock.Seek(masterClockTime);
                            track.clock.Play();
                            if (track.hasAudio && m_audioPlayback) {
                                m_audioPlayback->Resume(entity);
                            }
                        }
                    }

                    for (const auto& cb : m_callbacks) {
                        try {
                            cb(entity, videoComp.frameDataRGBA.data(), videoComp.width, videoComp.height);
                        }
                        catch (const std::exception& e) {
                            ANI_LOG_ERROR("[VideoPlaybackSystem] callback exception: %s", e.what());
                        }
                        catch (...) {
                            ANI_LOG_ERROR("[VideoPlaybackSystem] Unknown callback exception");
                        }
                    }
                }
            }
        }

        for (EntityID entity : endedEntities) {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it != m_tracks.end()) {
                HandleEndOfStream(entity, *it->second);
            }
        }
    }

    void VideoPlaybackSystem::HandleEndOfStream(EntityID entity, VideoTrack& track) {
        if (track.loop) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] Looping entity %u", entity);
            track.reachedEnd = false;
            track.stopped = false;
            track.paused = false;
            track.seeking = false;
            track.seekPending = false;
            track.restartPending = true;
        }
        else {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] End of stream for entity %u", entity);
            track.paused = true;
            track.reachedEnd = true;
            track.stopped = true;
            track.seeking = false;
            track.seekPending = false;
            track.clock.Pause();
            track.RequestStop();
            if (track.hasAudio && m_audioPlayback) {
                m_audioPlayback->Stop(entity);
            }
            NotifyPlaybackEnd(entity);
        }
    }

    void VideoPlaybackSystem::NotifyPlaybackEnd(EntityID entity) {
        for (const auto& cb : m_endCallbacks) {
            try { cb(entity); }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[VideoPlaybackSystem] Exception in playback-end callback: %s", e.what());
            }
            catch (...) {
                ANI_LOG_ERROR("[VideoPlaybackSystem] Unknown exception in playback-end callback");
            }
        }
    }

    void VideoPlaybackSystem::RemoveTrack(EntityID entity) {
        std::unique_ptr<VideoTrack> track;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it == m_tracks.end()) {
                ANI_LOG_TRACE("[VideoPlaybackSystem] RemoveTrack: no track for entity %u", entity);
                return;
            }
            track = std::move(it->second);
            m_tracks.erase(it);
        }
        ANI_LOG_DEBUG("[VideoPlaybackSystem] Removed track for entity %u", entity);
    }

    void VideoPlaybackSystem::RegisterVideoPlaybackCallback(const VideoPlaybackCallback& cb) {
        m_callbacks.push_back(cb);
    }

    void VideoPlaybackSystem::RegisterPlaybackEndCallback(const PlaybackEndCallback& cb) {
        m_endCallbacks.push_back(cb);
    }

    void VideoPlaybackSystem::Play(EntityID entity, bool loop) {
        ANI_LOG_INFO("[VideoPlaybackSystem] Play entity=%u loop=%s",
            entity, loop ? "true" : "false");

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            ANI_LOG_WARN("[VideoPlaybackSystem] Play: entity %u is invalid or lacks VideoComponent", entity);
            return;
        }
        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.frameCount <= 0) {
            ANI_LOG_WARN("[VideoPlaybackSystem] Play: entity %u has no loaded video", entity);
            return;
        }

        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);

        if (it != m_tracks.end()) {
            VideoTrack& track = *it->second;
            if (track.stopped || track.reachedEnd) {
                ANI_LOG_DEBUG("[VideoPlaybackSystem] Play: restarting stopped/ended track for entity %u", entity);
                track.stopped = false;
                track.reachedEnd = false;
                track.paused = false;
                track.seeking = false;
                track.seekPending = false;
                track.firstFrameAfterSeek = true;
                track.wasPlayingBeforeSeek = true;
                track.loop = loop;
                track.clock.Seek(0.0);
                track.clock.Play();
                track.RequestSeek(0.0, false);
                if (track.hasAudio && m_audioPlayback) {
                    m_audioPlayback->Seek(entity, 0.0);
                    m_audioPlayback->Play(entity, loop);
                    m_audioPlayback->SetVolume(entity, 1.0f);
                }
                return;
            }
            if (track.paused) {
                ANI_LOG_DEBUG("[VideoPlaybackSystem] Play: resuming paused track for entity %u", entity);
                track.paused = false;
                track.reachedEnd = false;
                track.seeking = false;
                track.seekPending = false;
                track.loop = loop;
                track.clock.Play();
                track.RequestResume();
                if (track.hasAudio && m_audioPlayback) {
                    m_audioPlayback->Resume(entity);
                }
                return;
            }
            ANI_LOG_TRACE("[VideoPlaybackSystem] Play: track already playing for entity %u", entity);
            return;
        }

        bool hasAudio = mgr.HasComponent<AudioComponent>(entity);
        double duration = videoComp.frameCount / videoComp.fps;
        double startTime = std::clamp(static_cast<double>(videoComp.currentFrame) / videoComp.fps,
            0.0, duration > 0.0 ? duration - 0.001 : 0.0);

        auto track = std::make_unique<VideoTrack>(this, mgr, entity, hasAudio, duration, videoComp.fps);
        track->loop = loop;
        track->paused = false;
        track->stopped = false;
        track->reachedEnd = false;
        track->seeking = false;
        track->seekPending = false;
        track->firstFrameAfterSeek = true;
        track->wasPlayingBeforeSeek = true;
        track->pendingSeekTimeStore = -1.0;
        track->restartPending = false;
        track->clock.Reset(startTime);
        track->clock.Play();
        track->StartThread();

        if (hasAudio && m_audioPlayback) {
            m_audioPlayback->Seek(entity, startTime);
            m_audioPlayback->Play(entity, loop);
            m_audioPlayback->SetVolume(entity, 1.0f);
        }

        m_tracks[entity] = std::move(track);
    }

    void VideoPlaybackSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] Pause: no track for entity %u", entity);
            return;
        }

        VideoTrack& track = *it->second;
        if (track.paused || track.stopped || track.reachedEnd) {
            ANI_LOG_TRACE("[VideoPlaybackSystem] Pause: track not active for entity %u", entity);
            return;
        }

        ANI_LOG_DEBUG("[VideoPlaybackSystem] Pausing entity %u", entity);

        track.paused = true;
        track.clock.Pause();
        track.RequestPause();

        if (track.hasAudio && m_audioPlayback) {
            m_audioPlayback->Pause(entity);
        }
    }

    void VideoPlaybackSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] Resume: no track for entity %u", entity);
            return;
        }

        VideoTrack& track = *it->second;
        if (!track.paused || track.stopped || track.reachedEnd) {
            ANI_LOG_TRACE("[VideoPlaybackSystem] Resume: track not paused for entity %u", entity);
            return;
        }

        ANI_LOG_DEBUG("[VideoPlaybackSystem] Resuming entity %u", entity);

        track.paused = false;
        track.clock.Play();
        track.RequestResume();

        if (track.hasAudio && m_audioPlayback) {
            m_audioPlayback->Resume(entity);
        }
    }

    void VideoPlaybackSystem::Stop(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] Stop: no track for entity %u", entity);
            return;
        }

        ANI_LOG_DEBUG("[VideoPlaybackSystem] Stopping entity %u", entity);

        VideoTrack& track = *it->second;
        track.stopped = true;
        track.paused = true;
        track.reachedEnd = false;
        track.seeking = false;
        track.seekPending = false;
        track.restartPending = false;
        track.clock.Seek(0.0);
        track.clock.Pause();
        track.RequestStop();
        track.ClearBuffer();

        if (track.hasAudio && m_audioPlayback && mgr.HasComponent<AudioComponent>(entity)) {
            m_audioPlayback->Stop(entity);
            m_audioPlayback->Seek(entity, 0.0);
        }

        if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity)) {
            track.RequestSeek(0.0, true);
        }

        NotifyPlaybackEnd(entity);
    }

    void VideoPlaybackSystem::Seek(EntityID entity, double time) {
        ANI_LOG_INFO("[VideoPlaybackSystem] Seek entity=%u time=%.3f", entity, time);

        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            ANI_LOG_WARN("[VideoPlaybackSystem] Seek: entity %u is invalid or lacks VideoComponent", entity);
            return;
        }
        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        if (!videoComp.fmtCtx || videoComp.frameCount <= 0) {
            ANI_LOG_WARN("[VideoPlaybackSystem] Seek: entity %u has no loaded video", entity);
            return;
        }

        double duration = videoComp.frameCount / videoComp.fps;
        time = std::clamp(time, 0.0, duration > 0.0 ? duration - 0.001 : 0.0);

        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);

        bool wasPlaying = false;
        bool trackExists = (it != m_tracks.end());

        if (trackExists) {
            VideoTrack& track = *it->second;

            wasPlaying = !track.paused && !track.stopped && !track.reachedEnd;

            if (wasPlaying) {
                track.paused = true;
                track.clock.Pause();
                track.RequestPause();
                if (track.hasAudio && m_audioPlayback) {
                    m_audioPlayback->Pause(entity);
                }
            }

            track.wasPlayingBeforeSeek = wasPlaying;
            track.clock.Seek(time);
            track.lastDisplayedPts = time;
            track.firstFrameAfterSeek = true;
            track.seeking = true;
            track.seekPending = true;
            track.pendingSeekTime = time;
            track.ClearBuffer();
            track.restartPending = false;

            track.RequestSeek(time, !wasPlaying);
        }
        else {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] Seek: creating paused track for entity %u", entity);
            bool hasAudio = mgr.HasComponent<AudioComponent>(entity);
            auto track = std::make_unique<VideoTrack>(this, mgr, entity, hasAudio, duration, videoComp.fps);
            track->paused = true;
            track->stopped = false;
            track->reachedEnd = false;
            track->seeking = false;
            track->seekPending = false;
            track->firstFrameAfterSeek = false;
            track->wasPlayingBeforeSeek = false;
            track->pendingSeekTimeStore = -1.0;
            track->restartPending = false;
            track->clock.Reset(time);
            track->StartThread();
            track->RequestSeek(time, true);
            if (hasAudio && m_audioPlayback) {
                m_audioPlayback->Seek(entity, time);
            }
            m_tracks[entity] = std::move(track);
            return;
        }

        if (trackExists && it->second->hasAudio && m_audioPlayback) {
            m_audioPlayback->Seek(entity, time);
        }
    }

    void VideoPlaybackSystem::SetSpeed(EntityID entity, float speed) {
        speed = std::clamp(speed, 0.1f, 4.0f);
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] SetSpeed: no track for entity %u", entity);
            return;
        }

        ANI_LOG_DEBUG("[VideoPlaybackSystem] SetSpeed entity=%u speed=%.2f", entity, speed);

        VideoTrack& track = *it->second;
        track.clock.SetSpeed(speed);
        if (track.hasAudio && m_audioPlayback) {
            m_audioPlayback->SetPlaybackSpeed(entity, speed);
        }
    }

    void VideoPlaybackSystem::SetVolume(EntityID entity, float volume) {
        volume = std::clamp(volume, 0.0f, 1.0f);
        if (mgr.HasComponent<AudioComponent>(entity) && m_audioPlayback) {
            ANI_LOG_DEBUG("[VideoPlaybackSystem] SetVolume entity=%u volume=%.2f", entity, volume);
            m_audioPlayback->SetVolume(entity, volume);
        }
    }

    bool VideoPlaybackSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && !it->second->paused && !it->second->reachedEnd && !it->second->stopped && !it->second->seeking;
    }

    bool VideoPlaybackSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && it->second->paused && !it->second->reachedEnd && !it->second->stopped;
    }

    double VideoPlaybackSystem::GetCurrentPosition(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) return 0.0;

        if (it->second->hasAudio && m_audioPlayback) {
            return m_audioPlayback->GetCurrentPosition(entity);
        }
        return it->second->clock.Now();
    }

    double VideoPlaybackSystem::GetDuration(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            return 0.0;
        }
        auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
        if (videoComp.fps <= 0.0) return 0.0;
        return videoComp.frameCount / videoComp.fps;
    }

    void VideoPlaybackSystem::Destroy() {
        ANI_LOG_INFO("[VideoPlaybackSystem] Destroying");
        m_destroying.store(true);

        std::unordered_map<EntityID, std::unique_ptr<VideoTrack>> tracks;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            tracks.swap(m_tracks);
        }

        for (auto& pair : tracks) {
            if (pair.second->hasAudio && m_audioPlayback && mgr.HasComponent<AudioComponent>(pair.first)) {
                m_audioPlayback->Stop(pair.first);
            }
        }

        tracks.clear();
    }

} // namespace ECS