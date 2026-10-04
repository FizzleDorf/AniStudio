// PreviewView.hpp
#pragma once

#include "BaseView.hpp"
#include "DiffusionCallbackUtils.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include "TextureComponent.hpp"
#include "TextureSystem.hpp"
#include "VideoSystem.hpp"
#include "AVSystem.hpp"
#include "PlaybackStateComponent.hpp"
#include "AudioComponent.hpp"
#include "SDcppSystem.hpp"
#include "Events.hpp"
#include "ECS.h"
#include "RepeatButtonUtils.hpp"
#include "IconFonts.hpp"
#include <imgui.h>
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <any>

namespace GUI {

    class PreviewView : public BaseView {
    public:
        PreviewView(ECS::EntityManager& entityMgr, ViewManager& viewMgr);
        ~PreviewView() override;

        static constexpr const char* GetMetadataJSON() {
            return R"({
                "displayName": "Preview",
                "category": "Diffusion",
                "description": "Live preview of the current diffusion generation."
            })";
        }

        void Init() override;
        void Update(const float deltaT) override;
        void Render() override;
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

        void ShowVideo(const std::string& path,
            const std::string& label = "preview",
            bool loop = true);

        void ClearVideo();
        bool HasVideo() const { return m_videoAttached; }

    private:
        enum class PreviewMode { None = 0, Proj, Tae, Vae };

        static int PreviewModeToInt(PreviewMode m);
        void ApplyModeToLibrary();

        void RefreshPreviewTextureIfNeeded(float deltaT);
        void PushPreviewFrameToComponent(const PreviewFrame& frame);

        void RefreshVideoTextureIfNeeded();
        void PushVideoFrameToComponent(const std::vector<uint8_t>& rgba,
            int w, int h);

        void DrawMenuBar();
        void DrawVideoControls();
        void DrawLivePreviewControls();
        void LoadModeFromSettings();
        void SaveModeToSettings();

        void SeekVideo(ECS::EntityID entity, double time);
        void PlayVideo(ECS::EntityID entity);
        void PauseVideo(ECS::EntityID entity);
        void StopVideo(ECS::EntityID entity);
        void SetVideoSpeed(ECS::EntityID entity, float speed);
        void SetVideoVolume(ECS::EntityID entity, float volume);
        bool HasAudioTrack(ECS::EntityID entity) const;
        void RenderAudioControls(ECS::EntityID entity);
        void RenderTimelineControls();
        std::string FormatTimecode(double seconds) const;

        void OnShowVideoInPreview(const std::any& data);
        ANI::Events::EventCallbackWithData m_showVideoCallback = nullptr;

        void OnSdcppTaskCompleted(ECS::SDCPPSystem::TaskType type,
            ECS::EntityID entityID,
            const std::string& fullPath);

        ECS::EntityID m_previewEntity = 0;
        bool          m_previewEntityHasComponent = false;
        uint64_t      m_lastSequence = 0;
        uint64_t      m_lastBatchSequence = 0;
        uint64_t      m_lastBatchFrameSeq = 0;

        int           m_liveFrameIndex = 0;
        double        m_liveFrameTimer = 0.0;
        double        m_liveFrameDuration = 1.0 / 16.0;
        bool          m_livePaused = false;

        ECS::EntityID m_videoPreviewEntity = 0;
        bool          m_videoPreviewEntityHasComponent = false;
        bool          m_videoAttached = false;
        bool          m_videoLoops = true;
        bool          m_videoAutoplay = true;
        int           m_lastVideoWidth = 0;
        int           m_lastVideoHeight = 0;
        uint64_t      m_lastVideoFingerprint = 0;

        bool          m_isSeeking = false;
        bool          m_pendingSeek = false;
        double        m_pendingSeekTime = 0.0;
        ECS::EntityID m_pendingSeekEntity = 0;
        float         m_playbackProgress = 0.0f;
        RepeatButtonHandler m_repeatButtonHandler;

        std::shared_ptr<ECS::TextureSystem> m_textureSystem;
        std::shared_ptr<ECS::VideoSystem>   m_videoSystem;

        PreviewMode m_mode = PreviewMode::Tae;
        int         m_interval = 1;
        bool        m_modeLoaded = false;
    };

}