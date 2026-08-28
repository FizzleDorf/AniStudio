#pragma once

#include "BaseMediaView.hpp"
#include "AudioComponent.hpp"
#include "AudioSystem.hpp"
#include <string>
#include <vector>
#include <chrono>

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
        ~AudioView() = default;

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

    protected:
        void OnMediaAdded(ECS::EntityID entity) override;
        void OnMediaRemoved(ECS::EntityID entity) override;
        std::string GetHistoryViewTypeName() const override;

    private:
        void RenderAudioInfo();
        void RenderControls();
        void RenderSelector();
        void RenderPlaybackControls();
        void RenderWaveform();
        void UpdateWaveformData();
        void PauseAllAudio();
        void PlayTestTone();

        std::vector<float> waveformData;
        float playbackProgress = 0.0f;
        bool showWaveform = true;
        bool m_testTonePlaying = false;
    };

} // namespace GUI