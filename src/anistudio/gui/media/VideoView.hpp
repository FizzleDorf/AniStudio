#pragma once

#include "BaseMediaView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include <vector>
#include <unordered_map>
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
        ~VideoView() = default;

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
        void RenderPlaybackControls();
        void RenderWaveform();
        void UpdateWaveformData();
        void PauseAllVideos();
        void ToggleFullscreen();
        void NextVideo();
        void PreviousVideo();

        std::string FormatTimecode(double seconds) const;

    private:
        enum class DisplayMode {
            FitToWindow,
            ActualResolution,
            Fullscreen
        };

        struct RepeatButtonState {
            double lastActionTime = 0.0;
            int repeatCount = 0;
        };

        std::unordered_map<ImGuiID, RepeatButtonState> m_repeatButtonStates;
        float GetVideoFPS(ECS::EntityID entity) const;
        bool ProcessRepeatButton(ImGuiID id, double initialDelay, double repeatRate, bool& outHeld);

        bool HasAudioTrack(ECS::EntityID entity) const;
        void RenderAudioControls(ECS::EntityID entity);

        double GetVideoCurrentTime(ECS::EntityID entity) const;
        double GetVideoDuration(ECS::EntityID entity) const;
        void SeekVideo(ECS::EntityID entity, double time);
        void PlayVideo(ECS::EntityID entity, bool loop);
        void PauseVideo(ECS::EntityID entity);
        void StopVideo(ECS::EntityID entity);
        void SetVideoSpeed(ECS::EntityID entity, float speed);
        void SetVideoVolume(ECS::EntityID entity, float volume);

        void SaveVideoWithAudio(ECS::EntityID entity, const std::string& filePath);
        void SaveVideoNoAudio(ECS::EntityID entity, const std::string& filePath);
        void SaveSelectedMediaNoAudio();
        void SaveSelectedMediaAsWithAudio();
        void SaveSelectedMediaAsNoAudio();

        bool m_useTimeSlider = true;
        bool m_isSeeking = false;
        bool m_pendingSeek = false;
        double m_pendingSeekTime = 0.0;
        ECS::EntityID m_pendingSeekEntity = 0;
        bool m_loopEnabled = false;
        bool m_showWaveform = true;
        bool m_autoplay = true;
        float m_playbackProgress = 0.0f;
        std::vector<float> m_waveformData;

        DisplayMode m_displayMode = DisplayMode::FitToWindow;
        bool m_isFullscreen = false;
        float m_videoAspectRatio = 1.0f;

        float m_fullscreenControlsTimer = 0.0f;
        bool m_showFullscreenControls = true;
        ImVec2 m_lastMousePos = ImVec2(0.0f, 0.0f);
    };

}