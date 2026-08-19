#pragma once

#include "BaseView.hpp"
#include "VideoComponent.hpp"
#include "VideoSystem.hpp"
#include <filesystem>

namespace GUI {

    class VideoHistoryView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Video History",
            "category": "Viewers",
            "description": "Shows thumbnails of loaded videos"
        })";
        }

        VideoHistoryView(ECS::EntityManager& mgr, ViewManager& vm);
        ~VideoHistoryView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void SetParentViewWorkspace(WorkspaceID parentWorkspace);

    private:
        std::shared_ptr<ECS::VideoSystem> videoSystem;
        std::vector<ECS::EntityID> videoEntities;
        WorkspaceID parentWorkspaceID;
        ECS::EntityID selectedEntityID;

        void RefreshEntities();
        void OnVideoAdded(ECS::EntityID entity);
        void OnVideoRemoved(ECS::EntityID entity);
        void SelectVideo(ECS::EntityID entity);
        void RenderVideoThumbnail(ECS::EntityID entityID, size_t index);
        std::string GetFileDate(const std::string& filePath);
        std::string FormatDate(const std::filesystem::file_time_type& ftime);
    };

} // namespace GUI