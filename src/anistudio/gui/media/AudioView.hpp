#pragma once
#include "Log.hpp"
#include "BaseMediaView.hpp"
#include "AudioComponent.hpp"
#include "AudioSystem.hpp"
#include "AVSystem.hpp"
#include "PlaybackStateComponent.hpp"
#include "PlaybackEvents.hpp"
#include "WaveformUtils.hpp"
#include <string>
#include <vector>

namespace GUI {

    class AudioView : public BaseMediaView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Audio Player",
            "category": "Viewers",
            "description": "A simple audio player"
        })";
        }

        AudioView(ECS::EntityManager& mgr, ViewManager& vm);
        ~AudioView() override;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void LoadMedia(const std::vector<std::string>& filePaths) override;
        void SaveSelectedMedia() override;
        void SaveSelectedMediaAs(const std::string& filePath) override;
        void RemoveSelectedMedia() override;
        void RefreshEntities() override;

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

        void OnMediaAdded(ECS::EntityID entity) override;
        void OnMediaRemoved(ECS::EntityID entity) override;
        std::string GetHistoryViewTypeName() const override;

    private:
        void RenderPlaybackControls();
        void RenderWaveform();
        void UpdateWaveformData();
        void PauseAllAudio();

        void PlayAudio(ECS::EntityID entity);
        void PauseAudio(ECS::EntityID entity);
        void StopAudio(ECS::EntityID entity);
        void SeekAudio(ECS::EntityID entity, double time);
        void SetAudioVolume(ECS::EntityID entity, float volume);

        WaveformData m_waveformData;
        WaveformRenderer m_waveformRenderer;
        float m_playbackProgress = 0.0f;
        float m_sliderValue = 0.0f;
        bool m_showWaveform = true;
        bool m_autoplay = true;

        ECS::PlaybackMode m_playbackMode = ECS::PlaybackMode::Cached;
        std::shared_ptr<ECS::AVSystem> m_avSystem;
        std::shared_ptr<ECS::AudioSystem> m_audioSystem;
    };

}