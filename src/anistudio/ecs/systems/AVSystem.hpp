#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"
#include "PlaybackStateComponent.hpp"
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <any>
#include <cstdint>
#include <string>

namespace ECS {

    class VideoSystem;
    class AudioSystem;
    class TextureSystem;

    class AVSystem : public BaseSystem {
    public:
        AVSystem(EntityManager& entityMgr);
        ~AVSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        EntityID LoadMedia(const std::string& filePath,
            TrackType type = TrackType::Video,
            PlaybackMode mode = PlaybackMode::Cached);

        void RemoveMedia(EntityID entity);
        void ReloadMedia(EntityID entity);
        void ClearCache(EntityID entity);
        void SetMode(EntityID entity, PlaybackMode mode);

        void Play(EntityID entity);
        void Pause(EntityID entity);
        void Stop(EntityID entity);
        void Seek(EntityID entity, double time);
        void SetSpeed(EntityID entity, float speed);
        void SetVolume(EntityID entity, float volume);
        void SetLooping(EntityID entity, bool loop);

        void PlayAll();
        void PauseAll();
        void StopAll();
        void SeekAll(double time);

        PlaybackState GetState(EntityID entity) const;
        double GetPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;
        bool   IsPlaying(EntityID entity) const;
        bool   IsPaused(EntityID entity) const;
        bool   IsMediaReady(EntityID entity) const;

        using StateCallback = std::function<void(EntityID, PlaybackState)>;
        void RegisterTrackStateCallback(StateCallback cb);

        using TextureCallback =
            std::function<void(EntityID, const uint8_t*, int, int, int)>;
        void SetVideoTextureCallback(TextureCallback cb);

        void onLoad(const std::any& data);
        void onPlay(const std::any& data);
        void onPause(const std::any& data);
        void onStop(const std::any& data);
        void onSeek(const std::any& data);
        void onSetSpeed(const std::any& data);
        void onSetVolume(const std::any& data);
        void onSetMode(const std::any& data);
        void onRemove(const std::any& data);

    private:
        std::shared_ptr<VideoSystem>   m_video;
        std::shared_ptr<AudioSystem>   m_audio;
        std::shared_ptr<TextureSystem> m_texture;

        std::vector<StateCallback> m_stateCallbacks;
        TextureCallback m_textureCallback;

        std::atomic<bool> m_destroying{ false };

        void EnsureSystems();
        bool IsVideoEntity(EntityID entity) const;
        bool IsAudioEntity(EntityID entity) const;
        void FireState(EntityID entity, PlaybackState state);

        void OnVideoLoaded(EntityID entity, bool ok);
        void OnAudioAdded(EntityID entity);
        void MaybeAttachSilentClock(EntityID entity);
        void ResumePlaybackIfRequested(EntityID entity);
    };

} // namespace ECS