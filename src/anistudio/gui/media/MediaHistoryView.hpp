// MediaHistoryView.hpp
#ifndef MEDIAHISTORYVIEW_HPP
#define MEDIAHISTORYVIEW_HPP

#include "BaseView.hpp"
#include "ContextMenuUtils.hpp"
#include "ThumbnailUtils.hpp"
#include "ThumbnailFilters.hpp"
#include "ImageComponent.hpp"
#include "VideoComponent.hpp"
#include <vector>
#include <unordered_map>
#include <limits>

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
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;

    private:
        ThumbnailFilters::Settings filterSettings;

        std::shared_ptr<ECS::ImageSystem> imageSystem;
        std::shared_ptr<ECS::VideoSystem> videoSystem;
        std::vector<ECS::EntityID> mediaEntities;
        ECS::EntityID selectedEntityID = 0;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;
        bool needsSort = true;

        void RefreshEntities();
        void RenderMenuBar();
        void RenderMediaGrid();
        void SelectMedia(ECS::EntityID entityID);
        void OnMediaAdded(ECS::EntityID entity);
        void OnMediaRemoved(ECS::EntityID entity);
        void UpdateSelectedAfterRemoval(ECS::EntityID removedEntity);
        void ApplyFiltersAndSort();
    };

}

#endif