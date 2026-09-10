#pragma once

#include "BaseSystem.hpp"
#include "EntityManager.hpp"
#include "PlaybackStateComponent.hpp"
#include "VideoComponent.hpp"
#include "AudioComponent.hpp"
#include "VideoSystem.hpp"
#include "VideoPlaybackSystem.hpp"
#include "AudioSystem.hpp"
#include "AudioPlaybackSystem.hpp"
#include "AVStreamingSystem.hpp"
#include "TextureSystem.hpp"
#include "VideoAudioSystem.hpp"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>
#include <memory>

namespace ECS {

    class MediaEngineSystem : public BaseSystem {
    public:
        MediaEngineSystem(EntityManager& entityMgr);
        ~MediaEngineSystem() override;

        void Start() override;
        void Update(float deltaT) override;
        void Destroy() override;

        void onLoad(const std::any& data);
        void onPlay(const std::any& data);
        void onPause(const std::any& data);
        void onStop(const std::any& data);
        void onSeek(const std::any& data);
        void onSetSpeed(const std::any& data);
        void onSetVolume(const std::any& data);
        void onRemove(const std::any& data);
        void onSetMode(const std::any& data);

        PlaybackState GetState(EntityID entity) const;
        double GetPosition(EntityID entity) const;
        double GetDuration(EntityID entity) const;

        void PlayAll();
        void PauseAll();
        void StopAll();
        void SeekAll(double time);

        void SetVideoTextureCallback(std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> cb);
        void RegisterTrackStateCallback(std::function<void(EntityID, PlaybackState)> cb);

        EntityID LoadMedia(const std::string& filePath, TrackType type = TrackType::Video, PlaybackMode mode = PlaybackMode::Cached);

    private:
        std::shared_ptr<VideoSystem> m_videoSystem;
        std::shared_ptr<VideoPlaybackSystem> m_videoPlayback;
        std::shared_ptr<AudioSystem> m_audioSystem;
        std::shared_ptr<AudioPlaybackSystem> m_audioPlayback;
        std::shared_ptr<AVStreamingSystem> m_streaming;
        std::shared_ptr<TextureSystem> m_textureSystem;
        std::shared_ptr<VideoAudioSystem> m_videoAudioSystem;

        std::function<void(EntityID, unsigned char*, int, int, int, GLuint*)> m_textureCallback;
        std::vector<std::function<void(EntityID, PlaybackState)>> m_stateCallbacks;

        void ensureSystems();
        void updateComponentState(EntityID entity);
        bool isVideo(EntityID entity) const;
        bool isAudio(EntityID entity) const;
        bool isMediaReady(EntityID entity) const;
    };

}