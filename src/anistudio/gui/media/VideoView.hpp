#pragma once

#include "BaseMediaView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include <vector>

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
        ECS::EntityID lastGeneratedVideoID;

        void OnMediaAdded(ECS::EntityID entity) override;
        void OnMediaRemoved(ECS::EntityID entity) override;
        std::string GetHistoryViewTypeName() const override;

        void RenderMenuBar();
        void RenderVideoInfo();
        void RenderControls();
        void RenderSelector();
        void RenderPlaybackControls();
        void RenderSelected();
        void RenderWaveform();
        void UpdateWaveformData();
        void PauseAllVideos();

    private:
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
        float m_playbackProgress = 0.0f;
        std::vector<float> m_waveformData;
    };

}