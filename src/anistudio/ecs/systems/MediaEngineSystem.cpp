#include "MediaEngineSystem.hpp"
#include "PlaybackEvents.hpp"
#include <iostream>

namespace ECS {

    MediaEngineSystem::MediaEngineSystem(EntityManager& entityMgr)
        : BaseSystem(entityMgr) {
        sysName = "MediaEngineSystem";
        AddComponentSignature<PlaybackStateComponent>();
        std::cout << "[MediaEngineSystem] Constructor called" << std::endl;
    }

    MediaEngineSystem::~MediaEngineSystem() {
        Destroy();
    }

    void MediaEngineSystem::Start() {
        ensureSystems();
        std::cout << "[MediaEngineSystem] Started" << std::endl;
    }

    void MediaEngineSystem::Update(float deltaT) {
        std::vector<EntityID> entities;
        for (auto e : mgr.GetAllEntities()) {
            if (mgr.HasComponent<PlaybackStateComponent>(e)) {
                entities.push_back(e);
            }
        }
        for (auto e : entities) {
            updateComponentState(e);
        }
    }

    void MediaEngineSystem::Destroy() {
    }

    void MediaEngineSystem::ensureSystems() {
        m_videoSystem = mgr.GetSystem<VideoSystem>();
        if (!m_videoSystem) {
            mgr.RegisterSystem<VideoSystem>();
            m_videoSystem = mgr.GetSystem<VideoSystem>();
        }

        m_videoPlayback = mgr.GetSystem<VideoPlaybackSystem>();
        if (!m_videoPlayback) {
            mgr.RegisterSystem<VideoPlaybackSystem>();
            m_videoPlayback = mgr.GetSystem<VideoPlaybackSystem>();
        }

        m_audioSystem = mgr.GetSystem<AudioSystem>();
        if (!m_audioSystem) {
            mgr.RegisterSystem<AudioSystem>();
            m_audioSystem = mgr.GetSystem<AudioSystem>();
        }

        m_audioPlayback = mgr.GetSystem<AudioPlaybackSystem>();
        if (!m_audioPlayback) {
            mgr.RegisterSystem<AudioPlaybackSystem>();
            m_audioPlayback = mgr.GetSystem<AudioPlaybackSystem>();
        }

        m_streaming = mgr.GetSystem<AVStreamingSystem>();
        if (!m_streaming) {
            mgr.RegisterSystem<AVStreamingSystem>();
            m_streaming = mgr.GetSystem<AVStreamingSystem>();
            m_streaming->Start();
        }

        m_textureSystem = mgr.GetSystem<TextureSystem>();
        if (!m_textureSystem) {
            mgr.RegisterSystem<TextureSystem>();
            m_textureSystem = mgr.GetSystem<TextureSystem>();
        }

        m_videoAudioSystem = mgr.GetSystem<VideoAudioSystem>();
        if (!m_videoAudioSystem) {
            mgr.RegisterSystem<VideoAudioSystem>();
            m_videoAudioSystem = mgr.GetSystem<VideoAudioSystem>();
        }
    }

    void MediaEngineSystem::SetVideoTextureCallback(std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> cb) {
        m_textureCallback = cb;
    }

    void MediaEngineSystem::RegisterTrackStateCallback(std::function<void(EntityID, PlaybackState)> cb) {
        m_stateCallbacks.push_back(cb);
    }

    bool MediaEngineSystem::isMediaReady(EntityID entity) const {
        if (!mgr.IsEntityValid(entity)) return false;

        bool hasMedia = false;

        if (mgr.HasComponent<VideoComponent>(entity)) {
            hasMedia = true;
            const auto& videoComp = mgr.GetComponent<VideoComponent>(entity);
            if (!videoComp.fmtCtx || videoComp.frameCount <= 0) {
                return false;
            }
        }

        if (mgr.HasComponent<AudioComponent>(entity)) {
            hasMedia = true;
            const auto& audioComp = mgr.GetComponent<AudioComponent>(entity);
            if (audioComp.isLoading.load()) {
                return false;
            }
            if (audioComp.pcmData.empty() && !audioComp.filePath.empty()) {
                return false;
            }
        }

        return hasMedia;
    }

    EntityID MediaEngineSystem::LoadMedia(const std::string& filePath, TrackType type, PlaybackMode mode) {
        std::cout << "[MediaEngineSystem] LoadMedia: " << filePath << std::endl;

        if (filePath.empty()) return 0;

        EntityID entity = mgr.AddNewEntity();
        mgr.AddComponent<PlaybackStateComponent>(entity);
        auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);
        state.filePath = filePath;
        state.mode = mode;
        state.trackType = type;
        state.isLoaded = false;
        state.state = PlaybackState::Stopped;

        if (type == TrackType::Both || type == TrackType::Video) {
            mgr.AddComponent<VideoComponent>(entity);
            auto& vc = mgr.GetComponent<VideoComponent>(entity);
            vc.filePath = filePath;
            size_t lastSlash = filePath.find_last_of("/\\");
            vc.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
            if (m_videoSystem) m_videoSystem->SetVideo(entity, filePath);
        }

        if (type == TrackType::Both || type == TrackType::Audio) {
            mgr.AddComponent<AudioComponent>(entity);
            auto& ac = mgr.GetComponent<AudioComponent>(entity);
            ac.filePath = filePath;
            size_t lastSlash = filePath.find_last_of("/\\");
            ac.fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;
            if (m_audioSystem) m_audioSystem->SetAudio(entity, filePath);
        }

        return entity;
    }

    void MediaEngineSystem::onLoad(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            std::cout << "[MediaEngineSystem] onLoad entity " << entity << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onLoad error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onPlay(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);

            if (!isMediaReady(entity)) {
                std::cout << "[MediaEngineSystem] onPlay entity " << entity
                    << " - media not ready yet, deferring" << std::endl;
                state.state = PlaybackState::Playing;
                state.isPaused = false;
                return;
            }

            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) {
                    m_videoPlayback->Play(entity, state.looping);
                }
                if (audio && m_audioPlayback) {
                    m_audioPlayback->Play(entity, state.looping);
                }
            }
            else {
                if (m_streaming) {
                    m_streaming->Play(entity, state.looping);
                }
            }

            state.state = PlaybackState::Playing;
            state.isPaused = false;

            if (video && mgr.HasComponent<VideoComponent>(entity)) {
                auto& vc = mgr.GetComponent<VideoComponent>(entity);
                vc.isPaused = false;
                vc.looping = state.looping;
            }
            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                auto& ac = mgr.GetComponent<AudioComponent>(entity);
                ac.looping = state.looping;
                ac.reachedEnd = false;
            }

            for (const auto& cb : m_stateCallbacks) {
                try { cb(entity, PlaybackState::Playing); }
                catch (...) {}
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onPlay error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onPause(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);
            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback && isMediaReady(entity)) {
                    m_videoPlayback->Pause(entity);
                }
                if (audio && m_audioPlayback && isMediaReady(entity)) {
                    m_audioPlayback->Pause(entity);
                }
            }
            else if (m_streaming) {
                m_streaming->Pause(entity);
            }

            state.state = PlaybackState::Paused;
            state.isPaused = true;

            if (video && mgr.HasComponent<VideoComponent>(entity)) {
                mgr.GetComponent<VideoComponent>(entity).isPaused = true;
            }
            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                mgr.GetComponent<AudioComponent>(entity).reachedEnd = false;
            }

            for (const auto& cb : m_stateCallbacks) {
                try { cb(entity, PlaybackState::Paused); }
                catch (...) {}
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onPause error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onStop(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);
            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) m_videoPlayback->Stop(entity);
                if (audio && m_audioPlayback) m_audioPlayback->Stop(entity);
            }
            else if (m_streaming) {
                m_streaming->Stop(entity);
            }

            state.state = PlaybackState::Stopped;
            state.isPaused = false;
            state.currentTime = 0.0;

            if (video && mgr.HasComponent<VideoComponent>(entity)) {
                auto& vc = mgr.GetComponent<VideoComponent>(entity);
                vc.isPaused = true;
                vc.currentTime = 0.0;
                vc.currentFrame = 0;
            }
            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                auto& ac = mgr.GetComponent<AudioComponent>(entity);
                ac.reachedEnd = false;
                ac.currentTime = 0.0;
            }

            for (const auto& cb : m_stateCallbacks) {
                try { cb(entity, PlaybackState::Stopped); }
                catch (...) {}
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onStop error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onSeek(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);

            if (!isMediaReady(entity)) {
                return;
            }

            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) m_videoPlayback->Seek(entity, state.currentTime);
                if (audio && m_audioPlayback) m_audioPlayback->Seek(entity, state.currentTime);
            }
            else if (m_streaming) {
                m_streaming->Seek(entity, state.currentTime);
            }

            if (video && mgr.HasComponent<VideoComponent>(entity)) {
                auto& vc = mgr.GetComponent<VideoComponent>(entity);
                vc.currentTime = state.currentTime;
                vc.currentFrame = static_cast<long long>(state.currentTime * state.fps);
            }
            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                mgr.GetComponent<AudioComponent>(entity).currentTime = state.currentTime;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onSeek error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onSetSpeed(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);

            if (!isMediaReady(entity)) return;

            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) m_videoPlayback->SetSpeed(entity, state.speed);
                if (audio && m_audioPlayback) m_audioPlayback->SetPlaybackSpeed(entity, state.speed);
            }
            else if (m_streaming) {
                m_streaming->SetSpeed(entity, state.speed);
            }

            if (video && mgr.HasComponent<VideoComponent>(entity)) {
                mgr.GetComponent<VideoComponent>(entity).playbackSpeed = state.speed;
            }
            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                mgr.GetComponent<AudioComponent>(entity).playbackSpeed = state.speed;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onSetSpeed error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onSetVolume(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);

            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) m_videoPlayback->SetVolume(entity, state.volume);
                if (audio && m_audioPlayback) m_audioPlayback->SetVolume(entity, state.volume);
            }
            else if (m_streaming) {
                m_streaming->SetVolume(entity, state.volume);
            }

            if (audio && mgr.HasComponent<AudioComponent>(entity)) {
                mgr.GetComponent<AudioComponent>(entity).volume = state.volume;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onSetVolume error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onRemove(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity)) return;

            bool video = isVideo(entity);
            bool audio = isAudio(entity);

            if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);

                if (state.mode == PlaybackMode::Cached) {
                    if (m_videoPlayback) m_videoPlayback->ClearCache(entity);
                    if (m_audioPlayback) m_audioPlayback->ClearCache(entity);
                }
                else {
                    if (m_streaming) m_streaming->ClearCache(entity);
                }
            }

            if (video && m_videoSystem) m_videoSystem->RemoveVideo(entity);
            if (audio && m_audioSystem) m_audioSystem->RemoveAudio(entity);

            mgr.DestroyEntity(entity);
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onRemove error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::onSetMode(const std::any& data) {
        try {
            auto eventData = std::any_cast<std::unordered_map<std::string, std::any>>(data);
            EntityID entity = std::any_cast<EntityID>(eventData.at("entityID"));

            if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
                return;
            }

            auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);
            PlaybackMode newMode = state.mode;

            std::cout << "[MediaEngineSystem] SetMode entity " << entity
                << " -> " << (newMode == PlaybackMode::Cached ? "Cached" : "Streaming")
                << std::endl;

            if (m_videoPlayback) m_videoPlayback->ClearCache(entity);
            if (m_audioPlayback) m_audioPlayback->ClearCache(entity);
            if (m_streaming)     m_streaming->ClearCache(entity);

            if (isVideo(entity) && m_videoSystem) m_videoSystem->ClearCache(entity);
            if (isAudio(entity) && m_audioSystem) m_audioSystem->ClearCache(entity);

            state.isLoaded = false;
            state.currentTime = 0.0;
            state.state = PlaybackState::Stopped;
            state.isPaused = false;

            if (newMode == PlaybackMode::Cached) {
                if (isVideo(entity) && m_videoSystem && mgr.HasComponent<VideoComponent>(entity)) {
                    auto& vc = mgr.GetComponent<VideoComponent>(entity);
                    if (vc.filePath.empty()) vc.filePath = state.filePath;
                    m_videoSystem->SetVideo(entity, vc.filePath);
                }
                if (isAudio(entity) && m_audioSystem && mgr.HasComponent<AudioComponent>(entity)) {
                    auto& ac = mgr.GetComponent<AudioComponent>(entity);
                    if (ac.filePath.empty()) ac.filePath = state.filePath;
                    m_audioSystem->SetAudio(entity, ac.filePath);
                }
            }
            else {
                if (m_streaming) m_streaming->Load(entity, state.filePath);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MediaEngineSystem] onSetMode error: " << e.what() << std::endl;
        }
    }

    void MediaEngineSystem::PlayAll() {
        for (auto entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                onPlay(ANI::PlaybackEvents::MakeEntityEvent(entity));
            }
        }
    }

    void MediaEngineSystem::PauseAll() {
        for (auto entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                onPause(ANI::PlaybackEvents::MakeEntityEvent(entity));
            }
        }
    }

    void MediaEngineSystem::StopAll() {
        for (auto entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                onStop(ANI::PlaybackEvents::MakeEntityEvent(entity));
            }
        }
    }

    void MediaEngineSystem::SeekAll(double time) {
        for (auto entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<PlaybackStateComponent>(entity)) {
                mgr.GetComponent<PlaybackStateComponent>(entity).currentTime = time;
                onSeek(ANI::PlaybackEvents::MakeEntityEvent(entity));
            }
        }
    }

    void MediaEngineSystem::updateComponentState(EntityID entity) {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity)) {
            return;
        }

        auto& state = mgr.GetComponent<PlaybackStateComponent>(entity);
        bool video = isVideo(entity);
        bool audio = isAudio(entity);

        bool ready = isMediaReady(entity);

        if (ready && state.state == PlaybackState::Playing && !state.isPaused) {
            bool alreadyPlaying = false;
            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) alreadyPlaying |= m_videoPlayback->IsPlaying(entity);
                if (audio && m_audioPlayback) alreadyPlaying |= m_audioPlayback->IsPlaying(entity);
            }
            else if (m_streaming) {
                alreadyPlaying = m_streaming->IsPlaying(entity);
            }

            if (!alreadyPlaying && !state.isLoaded) {
                state.isLoaded = true;
                if (state.mode == PlaybackMode::Cached) {
                    if (video && m_videoPlayback) m_videoPlayback->Play(entity, state.looping);
                    if (audio && m_audioPlayback) m_audioPlayback->Play(entity, state.looping);
                }
                else if (m_streaming) {
                    m_streaming->Play(entity, state.looping);
                }
            }
            else if (!state.isLoaded) {
                state.isLoaded = true;
            }
        }
        else if (ready && !state.isLoaded) {
            state.isLoaded = true;
        }

        if (state.isLoaded) {
            if (state.mode == PlaybackMode::Cached) {
                if (video && m_videoPlayback) {
                    state.currentTime = m_videoPlayback->GetCurrentPosition(entity);
                    double d = m_videoPlayback->GetDuration(entity);
                    if (d > 0.0) state.duration = d;

                    if (mgr.HasComponent<VideoComponent>(entity)) {
                        auto& vc = mgr.GetComponent<VideoComponent>(entity);
                        vc.currentTime = state.currentTime;
                        if (state.fps > 0.0)
                            vc.currentFrame = static_cast<long long>(state.currentTime * state.fps);
                    }
                }
                if (audio && m_audioPlayback) {
                    state.currentTime = m_audioPlayback->GetCurrentPosition(entity);
                    double d = m_audioPlayback->GetDuration(entity);
                    if (d > 0.0) state.duration = d;

                    if (mgr.HasComponent<AudioComponent>(entity)) {
                        mgr.GetComponent<AudioComponent>(entity).currentTime = state.currentTime;
                    }
                }
            }
            else if (m_streaming) {
                state.currentTime = m_streaming->GetPosition(entity);
                double d = m_streaming->GetDuration(entity);
                if (d > 0.0) state.duration = d;
            }
        }

        if (state.duration > 0.0 && state.currentTime >= state.duration) {
            if (state.state != PlaybackState::Stopped && state.state != PlaybackState::EndOfStream) {
                state.state = PlaybackState::EndOfStream;

                if (video && mgr.HasComponent<VideoComponent>(entity))
                    mgr.GetComponent<VideoComponent>(entity).isPaused = true;
                if (audio && mgr.HasComponent<AudioComponent>(entity))
                    mgr.GetComponent<AudioComponent>(entity).reachedEnd = true;

                for (const auto& cb : m_stateCallbacks) {
                    try { cb(entity, PlaybackState::EndOfStream); }
                    catch (...) {}
                }
            }
        }
    }

    bool MediaEngineSystem::isVideo(EntityID entity) const {
        return mgr.HasComponent<VideoComponent>(entity);
    }

    bool MediaEngineSystem::isAudio(EntityID entity) const {
        return mgr.HasComponent<AudioComponent>(entity);
    }

    PlaybackState MediaEngineSystem::GetState(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity))
            return PlaybackState::Stopped;
        return mgr.GetComponent<PlaybackStateComponent>(entity).state;
    }

    double MediaEngineSystem::GetPosition(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity))
            return 0.0;
        return mgr.GetComponent<PlaybackStateComponent>(entity).currentTime;
    }

    double MediaEngineSystem::GetDuration(EntityID entity) const {
        if (!mgr.IsEntityValid(entity) || !mgr.HasComponent<PlaybackStateComponent>(entity))
            return 0.0;
        return mgr.GetComponent<PlaybackStateComponent>(entity).duration;
    }

}