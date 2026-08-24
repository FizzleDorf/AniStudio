#pragma once

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include "ThumbnailUtils.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <vector>
#include <unordered_map>

namespace GUI {

    class MediaHistoryView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Media History",
            "category": "Viewers",
            "description": "Shows thumbnails of loaded images and videos"
        })";
        }

        MediaHistoryView(ECS::EntityManager& mgr, ViewManager& vm);
        ~MediaHistoryView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

    private:
        enum class MediaFilter { All, Images, Videos };
        MediaFilter currentFilter = MediaFilter::All;
        Thumbnail::DisplayMode currentDisplayMode = Thumbnail::DisplayMode::Detailed;

        std::shared_ptr<ECS::ImageSystem> imageSystem;
        std::shared_ptr<ECS::VideoSystem> videoSystem;
        std::vector<ECS::EntityID> mediaEntities;
        ECS::EntityID selectedEntityID = 0;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        void RefreshEntities();
        void RenderMenuBar();
        Thumbnail::ThumbnailData BuildThumbnailData(ECS::EntityID entityID);
        void SelectMedia(ECS::EntityID entityID);
        void OnMediaAdded(ECS::EntityID entity);
        void OnMediaRemoved(ECS::EntityID entity);
        void UpdateSelectedAfterRemoval(ECS::EntityID removedEntity);
    };

} // namespace GUI