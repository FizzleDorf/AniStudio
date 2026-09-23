#include "VideoSystem.hpp"
#include "AudioSystem.hpp"
#include "VideoUtils.hpp"
#include "Log.hpp"

#include <algorithm>
#include <filesystem>
#include <chrono>
#include <limits>
#include <cmath>
#include <cstring>

namespace ECS {

    VideoSystem::VideoSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "VideoSystem";
        AddComponentSignature<VideoComponent>();
    }

    VideoSystem::~VideoSystem() {
        Destroy();
    }

    void VideoSystem::Start() {
        auto all = mgr.GetAllEntities();
        for (auto e : all) {
            if (mgr.HasComponent<VideoComponent>(e)) {
                auto& vc = mgr.GetComponent<VideoComponent>(e);
                if (!vc.filePath.empty() && !vc.fmtCtx) {
                    PlaybackMode mode = PlaybackMode::Cached;
                    if (mgr.HasComponent<PlaybackStateComponent>(e)) {
                        mode = mgr.GetComponent<PlaybackStateComponent>(e).mode;
                    }
                    LoadVideoAsync(e, vc.filePath, mode);
                }
            }
        }
    }

    void VideoSystem::OnEntityDestroyed(EntityID entity) {
        std::unique_ptr<Track> detached;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it != m_tracks.end()) {
                detached = std::move(it->second);
                m_tracks.erase(it);
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            m_pendingRestores.erase(entity);
        }
    }

    void VideoSystem::Update(float deltaT) {
        (void)deltaT;
        if (m_destroying.load()) return;

        ProcessCompletedLoads();
        ProcessCompletedSaves();

        std::vector<EntityID> ended;

        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            for (auto& [entity, trackPtr] : m_tracks) {
                Track& track = *trackPtr;

                if (!mgr.IsEntityValid(entity)) continue;
                if (!mgr.HasComponent<VideoComponent>(entity)) continue;
                if (!mgr.HasComponent<PlaybackStateComponent>(entity)) continue;

                auto& vc = mgr.GetComponent<VideoComponent>(entity);
                auto& st = mgr.GetComponent<PlaybackStateComponent>(entity);

                if (track.stopped || track.paused) continue;
                if (track.seekPending.load()) continue;

                double master = st.currentTime;
                if (m_audio && m_audio->HasTrack(entity)) {
                    master = m_audio->GetCurrentPosition(entity);
                }
                if (master < 0.0) master = 0.0;

                st.currentTime = master;
                if (st.fps > 0.0)
                    st.currentFrame = static_cast<long long>(master * st.fps);

                if (track.pendingSeekTime >= 0.0) {
                    double t = track.pendingSeekTime;
                    track.pendingSeekTime = -1.0;
                    track.Push({ CmdType::Seek, t });
                }

                Frame frame;
                bool got = false;
                track.TryPopFrameForTime(master, frame, got);

                if (got) {
                    track.currentFrameRGBA = std::move(frame.data);
                    track.currentWidth = frame.width;
                    track.currentHeight = frame.height;

                    ANI_LOG_TRACE("[VideoSystem] Publish frame entity=%u pts=%.3f index=%lld master=%.3f",
                        entity, frame.pts, frame.index, master);

                    for (auto& [o, cb] : m_textureCallbacks) {
                        (void)o;
                        try {
                            cb(entity,
                                track.currentFrameRGBA.data(),
                                track.currentWidth,
                                track.currentHeight,
                                4);
                        }
                        catch (...) {}
                    }
                }

                if (vc.frameCount > 0 && vc.fps > 0.0) {
                    double dur = static_cast<double>(vc.frameCount) / vc.fps;
                    if (master >= dur - 0.001 && !track.endReached) {
                        if (st.looping) {
                            ANI_LOG_DEBUG("[VideoSystem] Loop entity=%u", entity);
                            track.Push({ CmdType::Seek, 0.0 });
                            track.firstFrameAfterSeek = true;
                            if (m_audio) {
                                m_audio->Seek(entity, 0.0);
                                m_audio->Play(entity, true);
                            }
                        }
                        else {
                            ended.push_back(entity);
                        }
                    }
                }
            }
        }

        for (auto e : ended) {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(e);
            if (it == m_tracks.end()) continue;
            Track& track = *it->second;
            if (track.endReached) continue;
            ANI_LOG_DEBUG("[VideoSystem] End of stream entity=%u", e);
            track.stopped = true;
            track.paused = true;
            track.endReached = true;
            track.Push({ CmdType::Stop });
            if (m_audio) m_audio->Stop(e);
            NotifyEnd(e);
        }
    }

    void VideoSystem::Destroy() {
        m_destroying.store(true);

        std::unordered_map<EntityID, std::unique_ptr<Track>> tracks;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            tracks.swap(m_tracks);
        }
        tracks.clear();

        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            m_pendingRestores.clear();
        }
        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveFutures.clear();
            m_savePaths.clear();
        }
    }

    void VideoSystem::LoadVideo(EntityID entity, const std::string& filePath, PlaybackMode mode) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) return;

        ANI_LOG_INFO("[VideoSystem] LoadVideo entity=%u path=%s mode=%d",
            entity, filePath.c_str(), static_cast<int>(mode));

        auto& vc = mgr.GetComponent<VideoComponent>(entity);
        vc.Unload();
        vc.filePath = filePath;
        size_t slash = filePath.find_last_of("/\\");
        vc.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;

        LoadVideoAsync(entity, filePath, mode);
    }

    void VideoSystem::RemoveVideo(EntityID entity) {
        ANI_LOG_INFO("[VideoSystem] RemoveVideo entity=%u", entity);
        OnEntityDestroyed(entity);

        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveFutures.erase(entity);
            m_savePaths.erase(entity);
        }
        {
            std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
            m_pendingLoads.erase(
                std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
                    [entity](const LoadingTask& t) { return t.entityID == entity; }),
                m_pendingLoads.end());
        }
        if (mgr.HasComponent<VideoComponent>(entity)) {
            mgr.GetComponent<VideoComponent>(entity).Unload();
        }
        NotifyVideoRemoved(entity);
    }

    void VideoSystem::ClearCache(EntityID entity) {
        ANI_LOG_DEBUG("[VideoSystem] ClearCache entity=%u", entity);
        OnEntityDestroyed(entity);

        {
            std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
            m_pendingLoads.erase(
                std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
                    [entity](const LoadingTask& t) { return t.entityID == entity; }),
                m_pendingLoads.end());
        }
        if (mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity)) {
            mgr.GetComponent<VideoComponent>(entity).Unload();
        }
    }

    void VideoSystem::SetMode(EntityID entity, PlaybackMode mode,
        double keepTime, bool wasPlaying) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) return;

        ANI_LOG_INFO("[VideoSystem] SetMode entity=%u mode=%d keepTime=%.3f wasPlaying=%d",
            entity, static_cast<int>(mode), keepTime, wasPlaying ? 1 : 0);

        {
            std::lock_guard<std::mutex> lock(m_restoreMutex);
            PendingRestore pr;
            pr.keepTime = keepTime;
            pr.wasPlaying = wasPlaying;
            m_pendingRestores[entity] = pr;
        }

        OnEntityDestroyed(entity);

        auto& vc = mgr.GetComponent<VideoComponent>(entity);
        std::string path = vc.filePath;
        vc.Unload();

        if (!path.empty()) {
            LoadVideoAsync(entity, path, mode);
        }
    }

    void VideoSystem::LoadVideoAsync(EntityID entity, const std::string& filePath, PlaybackMode mode) {
        auto pool = mgr.GetSystem<ThreadPoolSystem>();
        if (!pool) {
            ANI_LOG_ERROR("[VideoSystem] ThreadPoolSystem unavailable");
            NotifyLoadComplete(entity, false);
            return;
        }

        ANI_LOG_DEBUG("[VideoSystem] LoadVideoAsync entity=%u mode=%d",
            entity, static_cast<int>(mode));

        auto fut = pool->getIOPool().submit([filePath, entity]() -> LoadResult {
            return LoadVideoInBackground(filePath, entity);
            });

        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
        LoadingTask t;
        t.entityID = entity;
        t.filePath = filePath;
        t.mode = mode;
        t.future = std::move(fut);
        m_pendingLoads.push_back(std::move(t));
    }

    void VideoSystem::ProcessCompletedLoads() {
        std::lock_guard<std::recursive_mutex> lock(m_loadMutex);
        for (auto it = m_pendingLoads.begin(); it != m_pendingLoads.end();) {
            if (it->future.valid() &&
                it->future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                try {
                    LoadResult r = it->future.get();
                    if (r.success) ApplyLoadedVideo(std::move(r));
                    else NotifyLoadComplete(r.entityID, false);
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("[VideoSystem] load exception: %s", e.what());
                    NotifyLoadComplete(it->entityID, false);
                }
                it = m_pendingLoads.erase(it);
            }
            else ++it;
        }
    }

    void VideoSystem::ApplyLoadedVideo(LoadResult&& r) {
        EntityID e = r.entityID;
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<VideoComponent>(e)) {
            NotifyLoadComplete(e, false);
            return;
        }
        auto& vc = mgr.GetComponent<VideoComponent>(e);

        ANI_LOG_INFO("[VideoSystem] ApplyLoadedVideo entity=%u %dx%d fps=%.3f frames=%lld",
            e, r.width, r.height, r.fps, r.frameCount);

        vc.Unload();

        vc.fmtCtx.reset(r.fmtCtx);
        vc.codecCtx.reset(r.codecCtx);
        vc.swsCtx.reset(r.swsCtx);
        vc.frame.reset(r.frame);
        vc.pkt.reset(r.pkt);
        vc.videoStreamIndex = r.videoStreamIndex;
        vc.width = r.width;
        vc.height = r.height;
        vc.fps = r.fps;
        vc.frameCount = r.frameCount;
        vc.filePath = r.filePath;
        vc.fileName = r.fileName;
        vc.fileSize = r.fileSize;
        vc.fileDate = r.fileDate;
        vc.fileTime = r.fileTime;
        vc.hasExifData = r.hasExif;
        vc.hasLSBData = r.hasLSB;
        vc.hasAniStudioMetadata = r.hasAniStudio;

        r.fmtCtx = nullptr; r.codecCtx = nullptr; r.swsCtx = nullptr;
        r.frame = nullptr;  r.pkt = nullptr;

        NotifyVideoAdded(e);
        NotifyLoadComplete(e, true);
    }

    VideoSystem::LoadResult VideoSystem::LoadVideoInBackground(const std::string& filePath, EntityID entity) {
        LoadResult r;
        r.filePath = filePath;
        r.entityID = entity;
        size_t slash = filePath.find_last_of("/\\");
        r.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;

        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) < 0) return r;
        if (avformat_find_stream_info(fmt, nullptr) < 0) { avformat_close_input(&fmt); return r; }

        int vs = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vs = i; break; }
        }
        if (vs < 0) { avformat_close_input(&fmt); return r; }

        AVCodecParameters* par = fmt->streams[vs]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) { avformat_close_input(&fmt); return r; }

        AVCodecContext* cc = avcodec_alloc_context3(codec);
        if (!cc || avcodec_parameters_to_context(cc, par) < 0 || avcodec_open2(cc, codec, nullptr) < 0) {
            if (cc) avcodec_free_context(&cc);
            avformat_close_input(&fmt);
            return r;
        }

        r.width = par->width;
        r.height = par->height;
        r.videoStreamIndex = vs;

        AVStream* stream = fmt->streams[vs];
        double fps = av_q2d(stream->avg_frame_rate);
        if (fps <= 0) fps = av_q2d(stream->r_frame_rate);
        if (fps <= 0) fps = 30.0;
        r.fps = fps;

        double dur = 0.0;
        if (stream->duration != AV_NOPTS_VALUE) {
            dur = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
        }
        if (dur <= 0.0 && fmt->duration != AV_NOPTS_VALUE) {
            dur = static_cast<double>(fmt->duration) / AV_TIME_BASE;
        }
        r.frameCount = static_cast<long long>(dur * fps);
        if (r.frameCount <= 0) r.frameCount = 1000;

        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        if (!frame || !pkt) {
            if (pkt) av_packet_free(&pkt);
            if (frame) av_frame_free(&frame);
            avcodec_free_context(&cc);
            avformat_close_input(&fmt);
            return r;
        }

        SwsContext* sws = sws_getContext(r.width, r.height, cc->pix_fmt,
            r.width, r.height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        avformat_seek_file(fmt, -1, INT64_MIN, 0, 0, 0);
        avcodec_flush_buffers(cc);

        bool got = false;
        int maxAttempts = 200;
        while (av_read_frame(fmt, pkt) >= 0 && maxAttempts-- > 0) {
            if (pkt->stream_index == vs && avcodec_send_packet(cc, pkt) == 0) {
                while (avcodec_receive_frame(cc, frame) == 0) {
                    int w = frame->width, h = frame->height;
                    r.firstFrameRGBA.resize(static_cast<size_t>(w) * h * 4);
                    uint8_t* dst[1] = { r.firstFrameRGBA.data() };
                    int lines[1] = { w * 4 };
                    sws_scale(sws, frame->data, frame->linesize, 0, h, dst, lines);
                    got = true;
                    break;
                }
            }
            if (got) break;
            av_packet_unref(pkt);
        }
        av_packet_unref(pkt);

        if (!got) {
            av_packet_free(&pkt);
            av_frame_free(&frame);
            sws_freeContext(sws);
            avcodec_free_context(&cc);
            avformat_close_input(&fmt);
            return r;
        }

        avformat_seek_file(fmt, vs, INT64_MIN, 0, 0, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(cc);

        r.fmtCtx = fmt;
        r.codecCtx = cc;
        r.swsCtx = sws;
        r.frame = frame;
        r.pkt = pkt;

        try {
            r.fileSize = std::filesystem::file_size(filePath);
            auto ftime = std::filesystem::last_write_time(filePath);
            auto now = std::chrono::system_clock::now();
            auto diff = ftime - std::filesystem::file_time_type::clock::now();
            auto sys = now + std::chrono::duration_cast<std::chrono::system_clock::duration>(diff);
            std::time_t tt = std::chrono::system_clock::to_time_t(sys);
            std::tm tm = *std::localtime(&tt);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm); r.fileDate = buf;
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm); r.fileTime = buf;
        }
        catch (...) {}

        r.success = true;
        return r;
    }

    void VideoSystem::Play(EntityID entity, bool loop) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) return;
        auto& vc = mgr.GetComponent<VideoComponent>(entity);
        if (!vc.fmtCtx || vc.frameCount <= 0) {
            ANI_LOG_WARN("[VideoSystem] Play entity=%u but fmtCtx missing or frameCount=0", entity);
            return;
        }

        PlaybackMode mode = PlaybackMode::Cached;
        if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
            mode = mgr.GetComponent<PlaybackStateComponent>(entity).mode;
        }

        ANI_LOG_DEBUG("[VideoSystem] Play entity=%u loop=%d mode=%d",
            entity, loop ? 1 : 0, static_cast<int>(mode));

        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);

        if (it == m_tracks.end()) {
            auto track = std::make_unique<Track>(this, entity);
            track->loop = loop;
            track->mode = mode;
            track->paused = false;
            track->stopped = false;
            track->endReached = false;
            track->firstFrameAfterSeek = true;
            track->seekPending.store(true);
            track->StartThread();
            track->Push({ CmdType::Seek, 0.0 });
            m_tracks[entity] = std::move(track);
        }
        else {
            Track& track = *it->second;
            track.loop = loop;
            track.mode = mode;

            if (track.stopped || track.endReached) {
                track.stopped = false;
                track.endReached = false;
                track.paused = false;
                track.seeking = true;
                track.pendingSeekTime = 0.0;
                track.firstFrameAfterSeek = true;
                track.seekPending.store(true);
                track.ClearBuffer();
                track.Push({ CmdType::Seek, 0.0 });
                if (m_audio) m_audio->Seek(entity, 0.0);
            }
            else if (track.paused) {
                track.paused = false;
                track.Push({ CmdType::Resume });
            }
        }

        if (m_audio) m_audio->Play(entity, loop);
    }

    void VideoSystem::Pause(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) return;
        ANI_LOG_DEBUG("[VideoSystem] Pause entity=%u", entity);
        it->second->paused = true;
        it->second->Push({ CmdType::Pause });
        if (m_audio) m_audio->Pause(entity);
    }

    void VideoSystem::Resume(EntityID entity) {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) return;
        ANI_LOG_DEBUG("[VideoSystem] Resume entity=%u", entity);
        it->second->paused = false;
        it->second->stopped = false;
        it->second->endReached = false;
        it->second->Push({ CmdType::Resume });
        if (m_audio) m_audio->Resume(entity);
    }

    void VideoSystem::Stop(EntityID entity) {
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it != m_tracks.end()) {
                ANI_LOG_DEBUG("[VideoSystem] Stop entity=%u", entity);
                Track& t = *it->second;
                t.stopped = true;
                t.paused = true;
                t.endReached = false;
                t.seeking = false;
                t.pendingSeekTime = -1.0;
                t.firstFrameAfterSeek = true;
                t.lastDisplayedPts = -1.0;
                t.lastDisplayedIndex = -1;
                t.seekPending.store(false);
                t.Push({ CmdType::Stop });
                t.ClearBuffer();
            }
        }
        if (m_audio) m_audio->Stop(entity);
    }

    void VideoSystem::Seek(EntityID entity, double time) {
        ANI_LOG_DEBUG("[VideoSystem] Seek entity=%u time=%.3f", entity, time);

        Track* trackPtr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_tracksMutex);
            auto it = m_tracks.find(entity);
            if (it == m_tracks.end()) {
                auto track = std::make_unique<Track>(this, entity);
                track->paused = true;
                track->stopped = false;
                track->seeking = true;
                track->pendingSeekTime = time;
                track->firstFrameAfterSeek = true;
                track->seekPending.store(true);
                if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                    track->mode = mgr.GetComponent<PlaybackStateComponent>(entity).mode;
                }
                track->StartThread();
                track->Push({ CmdType::Seek, time });
                trackPtr = track.get();
                m_tracks[entity] = std::move(track);
            }
            else {
                Track& t = *it->second;
                t.seeking = true;
                t.pendingSeekTime = time;
                t.firstFrameAfterSeek = true;
                t.seekPending.store(true);
                t.ClearBuffer();
                t.Push({ CmdType::Seek, time });
                trackPtr = &t;
            }
        }
        if (m_audio) m_audio->Seek(entity, time);

        if (trackPtr) {
            auto deadline = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(500);
            while (trackPtr->seekPending.load() &&
                std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            if (trackPtr->seekPending.load()) {
                ANI_LOG_WARN("[VideoSystem] Seek entity=%u timed out waiting for worker", entity);
            }
        }
    }

    void VideoSystem::SetSpeed(EntityID entity, float speed) {
        ANI_LOG_DEBUG("[VideoSystem] SetSpeed entity=%u speed=%.2f", entity, speed);
        if (m_audio) m_audio->SetSpeed(entity, speed);
    }

    bool VideoSystem::IsPlaying(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && !it->second->paused && !it->second->stopped;
    }

    bool VideoSystem::IsPaused(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        return it != m_tracks.end() && it->second->paused && !it->second->stopped;
    }

    bool VideoSystem::IsLoading(EntityID entity) const {
        if (!mgr.IsEntityValid(entity)) return false;
        if (!mgr.HasComponent<PlaybackStateComponent>(entity)) return false;
        const auto& st = mgr.GetComponent<PlaybackStateComponent>(entity);
        return !st.isLoaded;
    }

    bool VideoSystem::IsSaving(EntityID entity) const {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        auto it = m_saveFutures.find(entity);
        if (it == m_saveFutures.end()) return false;
        return it->second.valid() &&
            it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout;
    }

    double VideoSystem::GetCurrentPosition(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity))
            return 0.0;
        return mgr.GetComponent<PlaybackStateComponent>(entity).currentTime;
    }

    double VideoSystem::GetDuration(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) return 0.0;
        auto& vc = mgr.GetComponent<VideoComponent>(entity);
        if (vc.fps <= 0.0) return 0.0;
        return static_cast<double>(vc.frameCount) / vc.fps;
    }

    std::vector<EntityID> VideoSystem::GetAllVideoEntities() const {
        std::vector<EntityID> result;
        for (auto e : mgr.GetAllEntities()) {
            if (mgr.HasComponent<VideoComponent>(e)) result.push_back(e);
        }
        return result;
    }

    bool VideoSystem::GetCurrentFrame(EntityID entity,
        std::vector<uint8_t>& outData,
        int& outWidth,
        int& outHeight) const {
        std::lock_guard<std::mutex> lock(m_tracksMutex);
        auto it = m_tracks.find(entity);
        if (it == m_tracks.end()) return false;
        const Track& t = *it->second;
        if (t.currentFrameRGBA.empty()) return false;
        outData = t.currentFrameRGBA;
        outWidth = t.currentWidth;
        outHeight = t.currentHeight;
        return true;
    }

    void VideoSystem::SaveVideoAsync(EntityID entity, const std::string& outputPath) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<VideoComponent>(entity)) {
            NotifySaveComplete(entity, false, "");
            return;
        }
        auto& vc = mgr.GetComponent<VideoComponent>(entity);
        if (!vc.fmtCtx || vc.videoStreamIndex < 0) {
            NotifySaveComplete(entity, false, "");
            return;
        }

        auto pool = mgr.GetSystem<ThreadPoolSystem>();
        if (!pool) { NotifySaveComplete(entity, false, ""); return; }

        SaveTask task;
        task.inputPath = vc.filePath;
        task.fps = static_cast<int>(vc.fps > 0 ? vc.fps : 24);
        task.width = vc.width;
        task.height = vc.height;
        task.frameCount = vc.frameCount;

        std::string path = outputPath;
        if (path.empty()) {
            auto p = std::filesystem::path(vc.filePath);
            path = (p.parent_path() / (p.stem().string() + "_saved.mp4")).string();
        }
        task.outputPath = path;

        std::lock_guard<std::mutex> lock(m_saveMutex);
        m_savePaths[entity] = path;
        m_saveFutures[entity] = pool->getIOPool().submit([task]() -> bool {
            return SaveVideoInBackground(task);
            });
    }

    bool VideoSystem::SaveVideoInBackground(const SaveTask& task) {
        AVFormatContext* fmt = nullptr;
        if (avformat_open_input(&fmt, task.inputPath.c_str(), nullptr, nullptr) < 0) return false;
        if (avformat_find_stream_info(fmt, nullptr) < 0) { avformat_close_input(&fmt); return false; }

        int vs = -1;
        for (unsigned i = 0; i < fmt->nb_streams; ++i) {
            if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vs = i; break; }
        }
        if (vs < 0) { avformat_close_input(&fmt); return false; }

        AVCodecParameters* par = fmt->streams[vs]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(par->codec_id);
        if (!codec) { avformat_close_input(&fmt); return false; }

        AVCodecContext* cc = avcodec_alloc_context3(codec);
        if (!cc || avcodec_parameters_to_context(cc, par) < 0 || avcodec_open2(cc, codec, nullptr) < 0) {
            if (cc) avcodec_free_context(&cc);
            avformat_close_input(&fmt);
            return false;
        }

        SwsContext* sws = sws_getContext(par->width, par->height, cc->pix_fmt,
            par->width, par->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();

        std::vector<Utils::VideoFrame> frames;
        frames.reserve(task.frameCount > 0 ? static_cast<size_t>(task.frameCount) : 1600);

        while (av_read_frame(fmt, pkt) >= 0) {
            if (pkt->stream_index == vs && avcodec_send_packet(cc, pkt) == 0) {
                while (avcodec_receive_frame(cc, frame) == 0) {
                    int w = frame->width, h = frame->height;
                    size_t sz = static_cast<size_t>(w) * h * 4;
                    auto buf = std::make_unique<unsigned char[]>(sz);
                    uint8_t* dst[1] = { buf.get() };
                    int lines[1] = { w * 4 };
                    sws_scale(sws, frame->data, frame->linesize, 0, h, dst, lines);

                    Utils::VideoFrame vf;
                    vf.width = w; vf.height = h; vf.channels = 4;
                    vf.data = buf.release();
                    frames.push_back(vf);
                }
            }
            av_packet_unref(pkt);
        }

        av_packet_free(&pkt);
        av_frame_free(&frame);
        sws_freeContext(sws);
        avcodec_free_context(&cc);
        avformat_close_input(&fmt);

        if (frames.empty()) return false;

        nlohmann::json meta;
        meta["fps"] = task.fps;
        meta["width"] = task.width;
        meta["height"] = task.height;
        meta["frameCount"] = frames.size();
        meta["originalFile"] = task.inputPath;

        bool ok = Utils::VideoUtils::EncodeFramesToVideo(frames, task.outputPath,
            task.fps, meta, nullptr);
        for (auto& f : frames) if (f.data) free((void*)f.data);
        return ok;
    }

    void VideoSystem::ProcessCompletedSaves() {
        std::lock_guard<std::mutex> lock(m_saveMutex);
        for (auto it = m_saveFutures.begin(); it != m_saveFutures.end();) {
            if (it->second.valid() &&
                it->second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                EntityID entity = it->first;
                bool ok = false;
                try { ok = it->second.get(); }
                catch (...) {}
                std::string path = m_savePaths.count(entity) ? m_savePaths[entity] : "";
                if (ok && mgr.IsEntityValid(entity) && mgr.HasComponent<VideoComponent>(entity)) {
                    auto& vc = mgr.GetComponent<VideoComponent>(entity);
                    vc.filePath = path;
                    vc.fileName = std::filesystem::path(path).filename().string();
                }
                ANI_LOG_INFO("[VideoSystem] Save complete entity=%u ok=%d path=%s",
                    entity, ok ? 1 : 0, path.c_str());
                NotifySaveComplete(entity, ok, path);
                m_savePaths.erase(entity);
                it = m_saveFutures.erase(it);
            }
            else ++it;
        }
    }

    bool VideoSystem::DecodeFrameForSave(VideoComponent& vc, long long idx, bool& isNew) {
        isNew = false;
        if (!vc.fmtCtx || vc.videoStreamIndex < 0) return false;
        if (idx < 0) idx = 0;
        if (idx >= vc.frameCount) idx = vc.frameCount - 1;

        double sec = static_cast<double>(idx) / (vc.fps > 0 ? vc.fps : 30.0);
        int64_t ts = static_cast<int64_t>(sec * AV_TIME_BASE);

        if (avformat_seek_file(vc.fmtCtx.get(), vc.videoStreamIndex,
            INT64_MIN, ts, ts, AVSEEK_FLAG_BACKWARD) < 0) return false;
        avcodec_flush_buffers(vc.codecCtx.get());

        AVPacket* pkt = vc.pkt.get();
        AVFrame* frame = vc.frame.get();
        int decoded = 0;
        int maxAttempts = 500;
        while (av_read_frame(vc.fmtCtx.get(), pkt) >= 0 && maxAttempts-- > 0) {
            if (pkt->stream_index == vc.videoStreamIndex &&
                avcodec_send_packet(vc.codecCtx.get(), pkt) == 0) {
                while (avcodec_receive_frame(vc.codecCtx.get(), frame) == 0) {
                    if (decoded == idx) {
                        av_packet_unref(pkt);
                        isNew = true;
                        return true;
                    }
                    ++decoded;
                }
            }
            av_packet_unref(pkt);
        }
        return false;
    }

    void VideoSystem::RegisterVideoAddedCallback(void* owner, const VideoCallback& cb) {
        m_addedCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::RegisterVideoRemovedCallback(void* owner, const VideoCallback& cb) {
        m_removedCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::RegisterVideoTextureCallback(void* owner, const VideoTextureCallback& cb) {
        m_textureCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::RegisterSaveCallback(void* owner, const SaveCallback& cb) {
        m_saveCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::RegisterLoadCallback(void* owner, const LoadCallback& cb) {
        m_loadCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::RegisterEndCallback(void* owner, const EndCallback& cb) {
        m_endCallbacks.emplace_back(owner, cb);
    }
    void VideoSystem::UnregisterCallbacksForOwner(void* owner) {
        auto drop = [owner](auto& v) {
            v.erase(std::remove_if(v.begin(), v.end(),
                [owner](const auto& p) { return p.first == owner; }), v.end());
            };
        drop(m_addedCallbacks);
        drop(m_removedCallbacks);
        drop(m_textureCallbacks);
        drop(m_saveCallbacks);
        drop(m_loadCallbacks);
        drop(m_endCallbacks);
    }

    void VideoSystem::NotifyVideoAdded(EntityID e) {
        for (auto& [o, cb] : m_addedCallbacks) { (void)o; try { cb(e); } catch (...) {} }
    }
    void VideoSystem::NotifyVideoRemoved(EntityID e) {
        for (auto& [o, cb] : m_removedCallbacks) { (void)o; try { cb(e); } catch (...) {} }
    }
    void VideoSystem::NotifyLoadComplete(EntityID e, bool ok) {
        for (auto& [o, cb] : m_loadCallbacks) { (void)o; try { cb(e, ok); } catch (...) {} }
    }
    void VideoSystem::NotifySaveComplete(EntityID e, bool ok, const std::string& p) {
        for (auto& [o, cb] : m_saveCallbacks) { (void)o; try { cb(e, ok, p); } catch (...) {} }
    }
    void VideoSystem::NotifyEnd(EntityID e) {
        for (auto& [o, cb] : m_endCallbacks) { (void)o; try { cb(e); } catch (...) {} }
    }

    VideoSystem::Track::Track(VideoSystem* o, EntityID e)
        : entity(e), owner(o) {
    }

    VideoSystem::Track::~Track() {
        running.store(false);
        cmdCV.notify_all();
        bufferCV.notify_all();
        if (thread.joinable()) thread.join();
    }

    void VideoSystem::Track::StartThread() {
        thread = std::thread([this]() { owner->WorkerLoop(*this); });
    }

    void VideoSystem::Track::Push(Cmd c) {
        {
            std::lock_guard<std::mutex> lock(cmdMutex);
            commands.push_back(c);
        }
        cmdCV.notify_all();
        bufferCV.notify_all();
    }

    void VideoSystem::Track::ClearBuffer() {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer.clear();
        bufferCV.notify_all();
    }

    size_t VideoSystem::Track::BufferTarget() const {
        return (mode == PlaybackMode::Streaming) ? 2u : 24u;
    }

    bool VideoSystem::Track::TryPopFrameForTime(double targetTime, Frame& out, bool& got) {
        got = false;
        std::lock_guard<std::mutex> lock(bufferMutex);
        while (!buffer.empty() && buffer.front().pts <= targetTime) {
            out = std::move(buffer.front());
            buffer.pop_front();
            got = true;
        }
        if (got) bufferCV.notify_all();
        return got;
    }

    bool VideoSystem::Track::TryPopAny(Frame& out) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (buffer.empty()) return false;
        out = std::move(buffer.front());
        buffer.pop_front();
        bufferCV.notify_all();
        return true;
    }

    bool VideoSystem::Track::DecodeOneFrame(Frame& out) {
        if (!owner->mgr.IsEntityValid(entity) ||
            !owner->mgr.HasComponent<VideoComponent>(entity)) return false;
        auto& vc = owner->mgr.GetComponent<VideoComponent>(entity);
        if (!vc.fmtCtx || vc.videoStreamIndex < 0 || !vc.pkt || !vc.frame) return false;

        AVPacket* pkt = vc.pkt.get();
        AVFrame* frame = vc.frame.get();
        AVStream* stream = vc.fmtCtx->streams[vc.videoStreamIndex];

        int attempts = 200;
        while (attempts-- > 0) {
            if (av_read_frame(vc.fmtCtx.get(), pkt) < 0) return false;
            if (pkt->stream_index != vc.videoStreamIndex) {
                av_packet_unref(pkt);
                continue;
            }
            bool produced = false;
            if (avcodec_send_packet(vc.codecCtx.get(), pkt) == 0) {
                while (avcodec_receive_frame(vc.codecCtx.get(), frame) == 0) {
                    int w = frame->width, h = frame->height;
                    if (w <= 0 || h <= 0) continue;

                    if (w != vc.width || h != vc.height || !vc.swsCtx) {
                        vc.width = w;
                        vc.height = h;
                        vc.swsCtx.reset(sws_getContext(w, h, vc.codecCtx->pix_fmt,
                            w, h, AV_PIX_FMT_RGBA,
                            SWS_BILINEAR, nullptr, nullptr, nullptr));
                    }

                    size_t sz = static_cast<size_t>(w) * h * 4;
                    out.data.resize(sz);
                    uint8_t* dst[1] = { out.data.data() };
                    int lines[1] = { w * 4 };
                    sws_scale(vc.swsCtx.get(), frame->data, frame->linesize, 0, h, dst, lines);

                    int64_t ts = (frame->best_effort_timestamp != AV_NOPTS_VALUE)
                        ? frame->best_effort_timestamp
                        : frame->pts;
                    double pts;
                    if (ts != AV_NOPTS_VALUE) {
                        pts = static_cast<double>(ts) * av_q2d(stream->time_base);
                    }
                    else {
                        pts = static_cast<double>(lastDisplayedIndex + 1) /
                            (vc.fps > 0 ? vc.fps : 30.0);
                    }

                    out.width = w;
                    out.height = h;
                    out.pts = pts;
                    out.index = static_cast<long long>(pts * (vc.fps > 0 ? vc.fps : 30.0) + 0.5);
                    lastDisplayedIndex = out.index;
                    produced = true;
                    break;
                }
            }
            av_packet_unref(pkt);
            if (produced) return true;
        }
        return false;
    }

    bool VideoSystem::Track::PerformSeek(double time) {
        if (!owner->mgr.IsEntityValid(entity) ||
            !owner->mgr.HasComponent<VideoComponent>(entity)) return false;
        auto& vc = owner->mgr.GetComponent<VideoComponent>(entity);
        if (!vc.fmtCtx || vc.videoStreamIndex < 0) return false;

        double dur = vc.fps > 0 ? static_cast<double>(vc.frameCount) / vc.fps : 0.0;
        time = std::clamp(time, 0.0, dur > 0 ? dur - 0.001 : 0.0);

        ANI_LOG_DEBUG("[VideoSystem::Track] PerformSeek entity=%u time=%.3f", entity, time);

        AVStream* stream = vc.fmtCtx->streams[vc.videoStreamIndex];
        int64_t ts = static_cast<int64_t>(time / av_q2d(stream->time_base));

        if (avformat_seek_file(vc.fmtCtx.get(), vc.videoStreamIndex,
            INT64_MIN, ts, ts, AVSEEK_FLAG_BACKWARD) < 0) {
            ANI_LOG_WARN("[VideoSystem::Track] avformat_seek_file failed entity=%u", entity);
            return false;
        }
        avcodec_flush_buffers(vc.codecCtx.get());

        long long targetIdx = static_cast<long long>(time * (vc.fps > 0 ? vc.fps : 30.0));
        lastDisplayedIndex = targetIdx - 1;

        Frame f;
        if (!DecodeOneFrame(f)) return false;

        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer.clear();
        buffer.push_back(std::move(f));
        bufferCV.notify_all();
        return true;
    }

    void VideoSystem::WorkerLoop(Track& track) {
        ANI_LOG_DEBUG("[VideoSystem::WorkerLoop] start entity=%u mode=%d",
            track.entity, static_cast<int>(track.mode));

        while (track.running.load()) {
            if (!mgr.IsEntityValid(track.entity) ||
                !mgr.HasComponent<VideoComponent>(track.entity)) {
                break;
            }

            std::vector<Cmd> cmds;
            {
                std::lock_guard<std::mutex> lock(track.cmdMutex);
                while (!track.commands.empty()) {
                    cmds.push_back(track.commands.front());
                    track.commands.pop_front();
                }
            }

            for (auto& c : cmds) {
                if (!track.running.load()) break;
                switch (c.type) {
                case CmdType::Seek: {
                    ANI_LOG_DEBUG("[VideoSystem::WorkerLoop] Seek cmd entity=%u time=%.3f",
                        track.entity, c.seekTime);
                    track.ClearBuffer();
                    if (track.PerformSeek(c.seekTime)) {
                        size_t target = track.BufferTarget();
                        Frame extra;
                        while (track.running.load() && track.DecodeOneFrame(extra)) {
                            bool pushed = false;
                            {
                                std::lock_guard<std::mutex> lock(track.bufferMutex);
                                if (track.buffer.size() < target) {
                                    track.buffer.push_back(std::move(extra));
                                    pushed = true;
                                }
                            }
                            if (!pushed) break;
                            track.bufferCV.notify_all();
                        }
                        ANI_LOG_DEBUG("[VideoSystem::WorkerLoop] Seek fill entity=%u buffer=%zu target=%zu",
                            track.entity, track.buffer.size(), target);
                    }
                    track.workerPaused.store(false);
                    track.seekPending.store(false);
                    break;
                }
                case CmdType::Pause:
                    track.workerPaused.store(true);
                    break;
                case CmdType::Resume:
                    track.workerPaused.store(false);
                    break;
                case CmdType::Stop:
                    track.workerPaused.store(true);
                    track.ClearBuffer();
                    break;
                case CmdType::Shutdown:
                    track.running.store(false);
                    break;
                }
            }

            if (!track.running.load()) break;

            if (track.workerPaused.load()) {
                std::unique_lock<std::mutex> lock(track.cmdMutex);
                track.cmdCV.wait_for(lock, std::chrono::milliseconds(20), [&] {
                    return !track.commands.empty() || !track.running.load();
                    });
                continue;
            }

            bool bufferFull = false;
            {
                std::lock_guard<std::mutex> lock(track.bufferMutex);
                bufferFull = track.buffer.size() >= track.BufferTarget();
            }
            if (bufferFull) {
                std::unique_lock<std::mutex> lock(track.bufferMutex);
                track.bufferCV.wait_for(lock, std::chrono::milliseconds(4), [&] {
                    return track.buffer.size() < track.BufferTarget() ||
                        !track.running.load();
                    });
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(track.cmdMutex);
                if (!track.commands.empty()) continue;
            }

            Frame f;
            if (!track.DecodeOneFrame(f)) {
                track.workerPaused.store(true);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(track.bufferMutex);
                if (track.buffer.size() < track.BufferTarget()) {
                    track.buffer.push_back(std::move(f));
                }
            }
            track.bufferCV.notify_all();
        }

        ANI_LOG_DEBUG("[VideoSystem::WorkerLoop] exit entity=%u", track.entity);
    }

} // namespace ECS