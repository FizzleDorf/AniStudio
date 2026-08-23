#pragma once

#include "BaseMediaView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"

namespace GUI {

    class VideoView : public BaseMediaView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Video View",
            "category": "Viewers",
            "description": "A simple video viewer"
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
        bool isPlaying;
        float playbackSpeed;
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
        void PauseAllVideos();
    };

}