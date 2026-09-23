#pragma once

#include "BaseMediaView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include "PlaybackStateComponent.hpp"
#include "AVSystem.hpp"
#include "PlaybackEvents.hpp"
#include "WaveformUtils.hpp"
#include "RepeatButtonUtils.hpp"
#include <vector>
#include <memory>
#include <imgui.h>

namespace GUI {

    class VideoView : public BaseMediaView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Video View",
            "category": "Viewers",
            "description": "A video viewer with audio waveform support"
        })";
        }

        VideoView(ECS::EntityManager& mgr, ViewManager& vm);
        ~VideoView() override;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void LoadMedia(const std::vector<std::string>& filePaths) override;
        void SaveSelectedMedia() override;
        void SaveSelectedMediaAs(const std::string& filePath) override;
        void RemoveSelectedMedia() override;
        void RefreshEntities() override;

        void LoadVideo(const std::string& filePath);
        bool IsHistoryVisible() const override;
        std::string GetSelectedFilePath() const override;

        void SetPlaybackMode(ECS::PlaybackMode mode);
        ECS::PlaybackMode GetPlaybackMode() const;

    protected:
        void RenderMenuBar() override;
        void RenderToolbar() override;
        void RenderControls() override;
        void RenderMediaInfo() override;
        void RenderSelector() override;
        void RenderMediaContent() override;

        ECS::EntityID lastGeneratedVideoID;

        void OnMediaAdded(ECS::EntityID entity) override;
        void OnMediaRemoved(ECS::EntityID entity) override;
        std::string GetHistoryViewTypeName() const override;

        void RenderFullscreen();
        void RenderTimeline();
        void RenderWaveform();
        void UpdateWaveformData();
        void PauseAllVideos();
        void ToggleFullscreen();
        void NextVideo();
        void PreviousVideo();
        std::string FormatTimecode(double seconds) const;

    private:
        bool HasAudioTrack(ECS::EntityID entity) const;
        void RenderAudioControls(ECS::EntityID entity);

        void SeekVideo(ECS::EntityID entity, double time);
        void PlayVideo(ECS::EntityID entity);
        void PauseVideo(ECS::EntityID entity);
        void StopVideo(ECS::EntityID entity);
        void SetVideoSpeed(ECS::EntityID entity, float speed);
        void SetVideoVolume(ECS::EntityID entity, float volume);

        void SaveVideoWithAudio(ECS::EntityID entity, const std::string& filePath);
        void SaveVideoNoAudio(ECS::EntityID entity, const std::string& filePath);
        void SaveSelectedMediaNoAudio();
        void SaveSelectedMediaAsWithAudio();
        void SaveSelectedMediaAsNoAudio();

        // Called on the main thread from VideoSystem once per displayed frame.
        // Creates the TextureComponent on demand and hands the frame to
        // TextureSystem for upload.
        void OnVideoFrame(ECS::EntityID entity, const uint8_t* data,
            int width, int height, int channels);

        bool m_showWaveform = true;
        bool m_autoplay = true;
        float m_playbackProgress = 0.0f;

        WaveformData m_waveformData;
        WaveformRenderer m_waveformRenderer;
        RepeatButtonHandler m_repeatButtonHandler;

        enum class DisplayMode {
            FitToWindow,
            ActualResolution,
            Fullscreen
        };
        DisplayMode m_displayMode = DisplayMode::FitToWindow;
        bool m_isFullscreen = false;
        float m_videoAspectRatio = 1.0f;

        float m_fullscreenControlsTimer = 0.0f;
        bool m_showFullscreenControls = true;
        ImVec2 m_lastMousePos = ImVec2(0.0f, 0.0f);

        bool m_isSeeking = false;
        bool m_pendingSeek = false;
        double m_pendingSeekTime = 0.0;
        ECS::EntityID m_pendingSeekEntity = 0;

        ECS::PlaybackMode m_playbackMode = ECS::PlaybackMode::Cached;
        std::shared_ptr<ECS::AVSystem> m_avSystem;
        std::shared_ptr<ECS::AudioSystem> m_audioSystem;
        std::shared_ptr<ECS::VideoSystem> m_videoSystem;
        std::shared_ptr<ECS::TextureSystem> m_textureSystem;
    };

}