#pragma once

#include "GUI.h"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include "FileDialogUtil.hpp"
#include "../events/Events.hpp"
#include <pch.h>

namespace GUI {

    class VideoView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Video View",
            "category": "Viewers",
            "description": "A simple video viewer."
        })";
        }

        VideoView(ECS::EntityManager& entityMgr);
        ~VideoView();

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

    private:
        ECS::EntityID selectedEntityID;
        int videoIndex;
        bool showHistory;
        size_t lastEntityCount;

        bool isPlaying;
        float playbackSpeed;

        float zoom;
        float offsetX;
        float offsetY;

        std::vector<ECS::EntityID> videoEntities;

        ECS::EntityID lastGeneratedVideoID;

        void RefreshVideoEntities();
        void RenderVideoInfo();
        void RenderControls();
        void RenderSelector();
        void RenderPlaybackControls();
        void RenderHistory();
        void RenderSelectedVideo();
        void DrawGrid(int videoWidth, int videoHeight);
        void SetZoom(float newZoom);
        void LoadVideos(const std::vector<std::string>& filePaths);
        void RemoveSelectedVideo();
        void PauseAllVideos();
        std::string TruncateFilename(const std::string& filename, float maxTextWidth);
    };

} // namespace GUI