#include "AVSystem.hpp"
#include "VideoSystem.hpp"
#include "AudioSystem.hpp"
#include "TextureSystem.hpp"
#include "PlaybackEvents.hpp"
#include "Log.hpp"

#include <iostream>
#include <filesystem>

namespace ECS {

    AVSystem::AVSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "AVSystem";
        AddComponentSignature<PlaybackStateComponent>();
    }

    AVSystem::~AVSystem() {
        Destroy();
    }

    void AVSystem::Start() {
        EnsureSystems();

        if (m_video) {
            m_video->RegisterEndCallback(this, [this](EntityID e) {
                if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
                auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
                if (st.state != PlaybackState::EndOfStream) {
                    ANI_LOG_DEBUG("[AVSystem] EndOfStream entity=%u", e);
                    st.state = PlaybackState::EndOfStream;
                    st.isPaused = true;
                    FireState(e, PlaybackState::EndOfStream);
                }
                });

            m_video->RegisterLoadCallback(this, [this](EntityID e, bool ok) {
                ANI_LOG_DEBUG("[AVSystem] Video load callback entity=%u ok=%d", e, ok ? 1 : 0);
                OnVideoLoaded(e, ok);
                });
        }

        if (m_audio) {
            m_audio->RegisterEndCallback(this, [this](EntityID e) {
                if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
                auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
                if (st.state != PlaybackState::EndOfStream) {
                    ANI_LOG_DEBUG("[AVSystem] Audio EndOfStream entity=%u", e);
                    st.state = PlaybackState::EndOfStream;
                    st.isPaused = true;
                    FireState(e, PlaybackState::EndOfStream);
                }
                });

            m_audio->RegisterAudioAddedCallback(this, [this](EntityID e) {
                ANI_LOG_DEBUG("[AVSystem] Audio added callback entity=%u", e);
                OnAudioAdded(e);
                });
        }

        ANI_LOG_INFO("[AVSystem] Started");
    }

    void AVSystem::Update(float deltaT) {
        (void)deltaT;
        if (m_destroying.load()) return;

        for (auto e : mgr.GetAllEntities()) {
            if (!mgr.HasComponent<PlaybackStateComponent>(e)) continue;
            auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

            if (!st.isLoaded && IsMediaReady(e)) st.isLoaded = true;

            if (m_audio && m_audio->HasTrack(e)) {
                double pos = m_audio->GetCurrentPosition(e);
                if (pos >= 0.0) st.currentTime = pos;
                if (st.fps > 0.0)
                    st.currentFrame = static_cast<long long>(pos * st.fps);
            }

            if (st.isPaused || st.state != PlaybackState::Playing) continue;
            if (st.duration > 0.0 && st.currentTime >= st.duration - 0.001 &&
                st.state != PlaybackState::EndOfStream) {
                ANI_LOG_DEBUG("[AVSystem] Update EndOfStream entity=%u currentTime=%.3f duration=%.3f",
                    e, st.currentTime, st.duration);
                st.state = PlaybackState::EndOfStream;
                st.isPaused = true;
                FireState(e, PlaybackState::EndOfStream);
            }
        }
    }

    void AVSystem::Destroy() {
        m_destroying.store(true);
        if (m_video) m_video->UnregisterCallbacksForOwner(this);
        if (m_audio) m_audio->UnregisterCallbacksForOwner(this);
    }

    void AVSystem::EnsureSystems() {
        m_video = mgr.GetSystem<VideoSystem>();
        if (!m_video) {
            mgr.RegisterSystem<VideoSystem>();
            m_video = mgr.GetSystem<VideoSystem>();
        }
        m_audio = mgr.GetSystem<AudioSystem>();
        if (!m_audio) {
            mgr.RegisterSystem<AudioSystem>();
            m_audio = mgr.GetSystem<AudioSystem>();
        }
        m_texture = mgr.GetSystem<TextureSystem>();
        if (!m_texture) {
            mgr.RegisterSystem<TextureSystem>();
            m_texture = mgr.GetSystem<TextureSystem>();
        }
        if (m_video) m_video->SetAudioSystem(m_audio.get());
    }

    bool AVSystem::IsVideoEntity(EntityID e) const {
        return mgr.HasComponent<VideoComponent>(e);
    }
    bool AVSystem::IsAudioEntity(EntityID e) const {
        return mgr.HasComponent<AudioComponent>(e);
    }

    void AVSystem::MaybeAttachSilentClock(EntityID e) {
        if (!m_audio) return;
        if (!mgr.IsEntityValid(e)) return;
        if (!mgr.HasComponent<VideoComponent>(e)) return;

        bool hasRealAudioStream = false;
        if (mgr.HasComponent<AudioComponent>(e)) {
            hasRealAudioStream = mgr.GetComponent<AudioComponent>(e).hasAudioStream;
        }
        if (hasRealAudioStream) return;
        if (m_audio->HasTrack(e)) return;

        double dur = 0.0;
        if (mgr.HasComponent<VideoComponent>(e)) {
            auto& vc = mgr.GetComponent<VideoComponent>(e);
            if (vc.frameCount > 0 && vc.fps > 0.0) {
                dur = static_cast<double>(vc.frameCount) / vc.fps;
            }
        }
        if (dur <= 0.0) return;

        m_audio->AddSilentTrack(e, dur);
        ANI_LOG_DEBUG("[AVSystem] Attached silent clock entity=%u duration=%.3fs", e, dur);
    }

    void AVSystem::ResumePlaybackIfRequested(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        // Only start playback when every present subsystem has finished
        // loading. Otherwise we'd call Play on a half-loaded entity and
        // the video or audio side would refuse.
        if (!IsMediaReady(e)) {
            ANI_LOG_DEBUG("[AVSystem] ResumePlaybackIfRequested entity=%u not ready yet", e);
            return;
        }

        st.isLoaded = true;

        if (st.state != PlaybackState::Playing) return;

        ANI_LOG_DEBUG("[AVSystem] ResumePlaybackIfRequested entity=%u keepTime=%.3f",
            e, st.currentTime);

        // Restore the clock before starting playback so the first frame
        // produced is at the right position.
        if (IsVideoEntity(e) && m_video) m_video->Seek(e, st.currentTime);
        if (IsAudioEntity(e) && m_audio) m_audio->Seek(e, st.currentTime);

        if (IsVideoEntity(e) && m_video) m_video->Play(e, st.looping);
        if (IsAudioEntity(e) && m_audio) m_audio->Play(e, st.looping);
    }

    void AVSystem::OnVideoLoaded(EntityID e, bool ok) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        if (mgr.HasComponent<VideoComponent>(e)) {
            auto& vc = mgr.GetComponent<VideoComponent>(e);
            if (vc.frameCount > 0 && vc.fps > 0.0) {
                st.duration = static_cast<double>(vc.frameCount) / vc.fps;
                st.fps = vc.fps;
                st.totalFrames = vc.frameCount;
                ANI_LOG_DEBUG("[AVSystem] Video metadata entity=%u frames=%lld fps=%.3f duration=%.3f",
                    e, vc.frameCount, vc.fps, st.duration);
            }
        }

        if (ok) MaybeAttachSilentClock(e);

        if (ok && IsMediaReady(e)) st.isLoaded = true;
        else if (!ok) st.isLoaded = false;

        if (ok) ResumePlaybackIfRequested(e);
    }

    void AVSystem::OnAudioAdded(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        if (mgr.HasComponent<AudioComponent>(e)) {
            auto& ac = mgr.GetComponent<AudioComponent>(e);
            if (ac.duration > 0.0) st.duration = ac.duration;
        }

        if (IsMediaReady(e)) st.isLoaded = true;

        ResumePlaybackIfRequested(e);
    }

    // ------------------------------------------------------------------
    // LoadMedia / AttachMedia
    // ------------------------------------------------------------------

    EntityID AVSystem::LoadMedia(const std::string& filePath,
        TrackType type,
        PlaybackMode mode) {
        if (filePath.empty()) return 0;

        EntityID e = mgr.AddNewEntity();
        if (!AttachMedia(e, filePath, type, mode)) {
            mgr.DestroyEntity(e);
            return 0;
        }
        return e;
    }

    bool AVSystem::AttachMedia(EntityID e,
        const std::string& filePath,
        TrackType type,
        PlaybackMode mode) {
        if (!mgr.IsEntityValid(e) || filePath.empty()) return false;

        // If this entity already had a decoder attached, tear it down first
        // so we do not leak Tracks or stale decode state when swapping media.
        if (mgr.HasComponent<VideoComponent>(e)) {
            if (m_video) m_video->RemoveVideo(e);
            mgr.GetComponent<VideoComponent>(e).Unload();
        }
        if (mgr.HasComponent<AudioComponent>(e)) {
            if (m_audio) m_audio->RemoveAudio(e);
        }

        // (Re)initialize the playback state on this entity.
        if (!mgr.HasComponent<PlaybackStateComponent>(e))
            mgr.AddComponent<PlaybackStateComponent>(e);
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
        st.filePath = filePath;
        st.mode = mode;
        st.trackType = type;
        st.state = PlaybackState::Stopped;
        st.isPaused = true;
        st.isLoaded = false;
        st.currentTime = 0.0;
        st.currentFrame = 0;

        bool fileHasVideo = false;
        bool fileHasAudio = false;
        double duration = 0.0;
        {
            AVFormatContext* fmt = nullptr;
            if (avformat_open_input(&fmt, filePath.c_str(), nullptr, nullptr) >= 0) {
                if (avformat_find_stream_info(fmt, nullptr) >= 0) {
                    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
                        auto t = fmt->streams[i]->codecpar->codec_type;
                        if (t == AVMEDIA_TYPE_VIDEO && !fileHasVideo) fileHasVideo = true;
                        if (t == AVMEDIA_TYPE_AUDIO && !fileHasAudio) fileHasAudio = true;
                    }
                    if (fmt->duration != AV_NOPTS_VALUE)
                        duration = static_cast<double>(fmt->duration) / AV_TIME_BASE;
                }
                avformat_close_input(&fmt);
            }
        }

        bool wantVideo = (type == TrackType::Video || type == TrackType::Both);
        bool wantAudio = (type == TrackType::Audio || type == TrackType::Both);

        // IMPORTANT: only add a plain VideoComponent if one is not already
        // present. For preview entities the caller has pre-added a
        // PreviewVideoComponent (which is-a VideoComponent), so this guard
        // ensures we reuse it instead of slapping on a second one and
        // destroying the preview marker.
        if (wantVideo && fileHasVideo) {
            if (!mgr.HasComponent<VideoComponent>(e))
                mgr.AddComponent<VideoComponent>(e);
            auto& vc = mgr.GetComponent<VideoComponent>(e);
            vc.filePath = filePath;
            size_t slash = filePath.find_last_of("/\\");
            vc.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;
            if (m_video) m_video->LoadVideo(e, filePath, mode);
        }

        if (wantAudio && fileHasAudio) {
            if (!mgr.HasComponent<AudioComponent>(e))
                mgr.AddComponent<AudioComponent>(e);
            auto& ac = mgr.GetComponent<AudioComponent>(e);
            ac.filePath = filePath;
            ac.hasAudioStream = true;
            size_t slash = filePath.find_last_of("/\\");
            ac.fileName = (slash != std::string::npos) ? filePath.substr(slash + 1) : filePath;
            if (m_audio) m_audio->LoadAudio(e, filePath, mode);
        }

        st.duration = duration;

        ANI_LOG_INFO("[AVSystem] AttachMedia entity=%u path=%s mode=%d hasVideo=%d hasAudio=%d",
            e, filePath.c_str(), static_cast<int>(mode), fileHasVideo, fileHasAudio);
        return true;
    }

    // ------------------------------------------------------------------
    // Everything below is unchanged from your version.
    // ------------------------------------------------------------------

    void AVSystem::RemoveMedia(EntityID e) {
        if (!mgr.IsEntityValid(e)) return;
        ANI_LOG_INFO("[AVSystem] RemoveMedia entity=%u", e);
        if (m_video) m_video->RemoveVideo(e);
        if (m_audio) m_audio->RemoveAudio(e);
        mgr.DestroyEntity(e);
    }

    void AVSystem::ClearCache(EntityID e) {
        ANI_LOG_DEBUG("[AVSystem] ClearCache entity=%u", e);
        if (m_video) m_video->ClearCache(e);
        if (m_audio) m_audio->ClearCache(e);
    }

    void AVSystem::SetMode(EntityID e, PlaybackMode mode) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
        if (st.mode == mode) {
            ANI_LOG_DEBUG("[AVSystem] SetMode entity=%u already in mode=%d",
                e, static_cast<int>(mode));
            return;
        }

        double keepTime = st.currentTime;
        bool wasPlaying = (st.state == PlaybackState::Playing);

        ANI_LOG_INFO("[AVSystem] SetMode entity=%u from=%d to=%d keepTime=%.3f wasPlaying=%d",
            e, static_cast<int>(st.mode), static_cast<int>(mode),
            keepTime, wasPlaying ? 1 : 0);

        // Store the target mode and desired restore state on the component.
        // The load callbacks will read these when the reload finishes.
        st.mode = mode;
        st.state = wasPlaying ? PlaybackState::Playing : PlaybackState::Stopped;
        st.isPaused = !wasPlaying;
        st.currentTime = keepTime;
        st.currentFrame = static_cast<long long>(keepTime * st.fps);
        st.isLoaded = false;

        ReloadMedia(e);
    }

    void AVSystem::ReloadMedia(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        // Stop both subsystems first so workers and audio producers are
        // joined before we tear down decoder resources.
        if (IsVideoEntity(e) && m_video) m_video->Stop(e);
        if (IsAudioEntity(e) && m_audio) m_audio->Stop(e);

        // Capture the file paths before unloading, because Unload clears
        // filePath on the component in some implementations.
        std::string videoPath;
        if (mgr.HasComponent<VideoComponent>(e)) {
            videoPath = mgr.GetComponent<VideoComponent>(e).filePath;
        }
        std::string audioPath;
        if (mgr.HasComponent<AudioComponent>(e)) {
            audioPath = mgr.GetComponent<AudioComponent>(e).filePath;
        }

        // Unload decoders, PCM, tracks, everything.
        if (IsVideoEntity(e) && m_video) m_video->ClearCache(e);
        if (IsAudioEntity(e) && m_audio) m_audio->ClearCache(e);

        // Re-issue loads in the new mode. These are async; the load
        // callbacks (OnVideoLoaded / OnAudioAdded) will fire when each
        // subsystem is ready, and they in turn call ResumePlaybackIfRequested
        // which checks that both are ready before restoring position.
        if (!videoPath.empty() && IsVideoEntity(e) && m_video) {
            m_video->LoadVideo(e, videoPath, st.mode);
        }
        if (!audioPath.empty() && IsAudioEntity(e) && m_audio) {
            m_audio->LoadAudio(e, audioPath, st.mode);
        }
    }

    void AVSystem::Play(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        ANI_LOG_DEBUG("[AVSystem] Play entity=%u", e);

        st.state = PlaybackState::Playing;
        st.isPaused = false;

        if (!IsMediaReady(e)) {
            ANI_LOG_DEBUG("[AVSystem] Play entity=%u media not ready yet, deferring", e);
            FireState(e, PlaybackState::Playing);
            return;
        }

        if (IsVideoEntity(e) && m_video) m_video->Play(e, st.looping);
        if (IsAudioEntity(e) && m_audio) m_audio->Play(e, st.looping);

        FireState(e, PlaybackState::Playing);
    }

    void AVSystem::Pause(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        ANI_LOG_DEBUG("[AVSystem] Pause entity=%u", e);

        if (IsVideoEntity(e) && m_video) m_video->Pause(e);
        if (IsAudioEntity(e) && m_audio) m_audio->Pause(e);

        st.state = PlaybackState::Paused;
        st.isPaused = true;
        FireState(e, PlaybackState::Paused);
    }

    void AVSystem::Stop(EntityID e) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        ANI_LOG_DEBUG("[AVSystem] Stop entity=%u", e);

        if (IsVideoEntity(e) && m_video) m_video->Stop(e);
        if (IsAudioEntity(e) && m_audio) m_audio->Stop(e);

        st.state = PlaybackState::Stopped;
        st.isPaused = false;
        st.currentTime = 0.0;
        st.currentFrame = 0;
        FireState(e, PlaybackState::Stopped);
    }

    void AVSystem::Seek(EntityID e, double time) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);

        ANI_LOG_DEBUG("[AVSystem] Seek entity=%u time=%.3f", e, time);

        if (IsVideoEntity(e) && m_video) m_video->Seek(e, time);
        if (IsAudioEntity(e) && m_audio) m_audio->Seek(e, time);

        st.currentTime = time;
        if (st.fps > 0.0)
            st.currentFrame = static_cast<long long>(time * st.fps);
    }

    void AVSystem::SetSpeed(EntityID e, float speed) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
        st.speed = speed;

        ANI_LOG_DEBUG("[AVSystem] SetSpeed entity=%u speed=%.2f", e, speed);

        if (IsVideoEntity(e) && m_video) m_video->SetSpeed(e, speed);
        if (IsAudioEntity(e) && m_audio) m_audio->SetSpeed(e, speed);
    }

    void AVSystem::SetVolume(EntityID e, float volume) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        auto& st = mgr.GetComponent<PlaybackStateComponent>(e);
        st.volume = volume;

        ANI_LOG_DEBUG("[AVSystem] SetVolume entity=%u volume=%.2f", e, volume);

        if (m_audio) m_audio->SetVolume(e, volume);
    }

    void AVSystem::SetLooping(EntityID e, bool loop) {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return;
        ANI_LOG_DEBUG("[AVSystem] SetLooping entity=%u loop=%d", e, loop ? 1 : 0);
        mgr.GetComponent<PlaybackStateComponent>(e).looping = loop;
    }

    void AVSystem::PlayAll() {
        for (auto e : mgr.GetAllEntities())
            if (mgr.HasComponent<PlaybackStateComponent>(e)) Play(e);
    }
    void AVSystem::PauseAll() {
        for (auto e : mgr.GetAllEntities())
            if (mgr.HasComponent<PlaybackStateComponent>(e)) Pause(e);
    }
    void AVSystem::StopAll() {
        for (auto e : mgr.GetAllEntities())
            if (mgr.HasComponent<PlaybackStateComponent>(e)) Stop(e);
    }
    void AVSystem::SeekAll(double time) {
        for (auto e : mgr.GetAllEntities())
            if (mgr.HasComponent<PlaybackStateComponent>(e)) Seek(e, time);
    }

    PlaybackState AVSystem::GetState(EntityID e) const {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e))
            return PlaybackState::Stopped;
        return mgr.GetComponent<PlaybackStateComponent>(e).state;
    }

    double AVSystem::GetPosition(EntityID e) const {
        if (m_audio) return m_audio->GetCurrentPosition(e);
        return 0.0;
    }

    double AVSystem::GetDuration(EntityID e) const {
        if (!mgr.IsEntityValid(e) || !mgr.HasComponent<PlaybackStateComponent>(e)) return 0.0;
        return mgr.GetComponent<PlaybackStateComponent>(e).duration;
    }

    bool AVSystem::IsPlaying(EntityID e) const {
        if (mgr.HasComponent<PlaybackStateComponent>(e))
            return mgr.GetComponent<PlaybackStateComponent>(e).state == PlaybackState::Playing;
        return false;
    }
    bool AVSystem::IsPaused(EntityID e) const {
        if (mgr.HasComponent<PlaybackStateComponent>(e))
            return mgr.GetComponent<PlaybackStateComponent>(e).state == PlaybackState::Paused;
        return false;
    }

    bool AVSystem::IsMediaReady(EntityID e) const {
        if (!mgr.IsEntityValid(e)) return false;
        bool any = false;

        if (mgr.HasComponent<VideoComponent>(e)) {
            any = true;
            auto& vc = mgr.GetComponent<VideoComponent>(e);
            if (!vc.fmtCtx || vc.frameCount <= 0) return false;
        }
        if (mgr.HasComponent<AudioComponent>(e)) {
            any = true;
            auto& ac = mgr.GetComponent<AudioComponent>(e);
            if (ac.hasAudioStream && ac.pcmData.empty() && !ac.HasDecoder()) return false;
        }
        if (!any && m_audio) {
            if (m_audio->GetDuration(e) > 0.0) return true;
        }
        return any;
    }

    void AVSystem::RegisterTrackStateCallback(StateCallback cb) {
        m_stateCallbacks.push_back(std::move(cb));
    }

    void AVSystem::SetVideoTextureCallback(TextureCallback cb) {
        m_textureCallback = std::move(cb);
        if (m_video) {
            m_video->RegisterVideoTextureCallback(this,
                [this](EntityID e, const uint8_t* d, int w, int h, int c) {
                    if (m_textureCallback) m_textureCallback(e, d, w, h, c);
                });
        }
    }

    void AVSystem::FireState(EntityID e, PlaybackState s) {
        for (auto& cb : m_stateCallbacks) {
            try { cb(e, s); }
            catch (...) {}
        }
    }

    void AVSystem::onLoad(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            std::string path = std::any_cast<std::string>(m.at("filePath"));
            TrackType type = m.count("trackType")
                ? std::any_cast<TrackType>(m.at("trackType")) : TrackType::Video;
            PlaybackMode mode = m.count("mode")
                ? std::any_cast<PlaybackMode>(m.at("mode")) : PlaybackMode::Cached;
            LoadMedia(path, type, mode);
        }
        catch (const std::exception& ex) {
            ANI_LOG_ERROR("[AVSystem] onLoad: %s", ex.what());
        }
    }
    void AVSystem::onPlay(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            Play(std::any_cast<EntityID>(m.at("entityID")));
        }
        catch (...) {}
    }
    void AVSystem::onPause(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            Pause(std::any_cast<EntityID>(m.at("entityID")));
        }
        catch (...) {}
    }
    void AVSystem::onStop(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            Stop(std::any_cast<EntityID>(m.at("entityID")));
        }
        catch (...) {}
    }
    void AVSystem::onSeek(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID e = std::any_cast<EntityID>(m.at("entityID"));
            double t = std::any_cast<double>(m.at("time"));
            Seek(e, t);
        }
        catch (...) {}
    }
    void AVSystem::onSetSpeed(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID e = std::any_cast<EntityID>(m.at("entityID"));
            float s = std::any_cast<float>(m.at("speed"));
            SetSpeed(e, s);
        }
        catch (...) {}
    }
    void AVSystem::onSetVolume(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID e = std::any_cast<EntityID>(m.at("entityID"));
            float v = std::any_cast<float>(m.at("volume"));
            SetVolume(e, v);
        }
        catch (...) {}
    }
    void AVSystem::onSetMode(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID e = std::any_cast<EntityID>(m.at("entityID"));
            PlaybackMode mode = std::any_cast<PlaybackMode>(m.at("mode"));
            SetMode(e, mode);
        }
        catch (const std::exception& ex) {
            ANI_LOG_ERROR("[AVSystem] onSetMode: %s", ex.what());
        }
    }
    void AVSystem::onRemove(const std::any& data) {
        try {
            auto m = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            RemoveMedia(std::any_cast<EntityID>(m.at("entityID")));
        }
        catch (...) {}
    }

} // namespace ECS