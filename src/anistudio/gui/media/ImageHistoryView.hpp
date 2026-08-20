#ifndef IMAGEHISTORYVIEW_HPP
#define IMAGEHISTORYVIEW_HPP

#include "BaseView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "ContextMenuUtils.hpp"
#include <filesystem>

namespace GUI {

    class ImageHistoryView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Image History",
            "category": "Viewers",
            "description": "Shows thumbnails of loaded images"
        })";
        }

        ImageHistoryView(ECS::EntityManager& mgr, ViewManager& vm);
        ~ImageHistoryView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void SetParentViewWorkspace(WorkspaceID parentWorkspace);

    private:
        std::shared_ptr<ECS::ImageSystem> imageSystem;
        std::vector<ECS::EntityID> imageEntities;
        WorkspaceID parentWorkspaceID;
        ECS::EntityID selectedEntityID;
        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        void RefreshEntities();
        void OnImageAdded(ECS::EntityID entity);
        void OnImageRemoved(ECS::EntityID entity);
        void SelectImage(ECS::EntityID entity);
        void RenderImageThumbnail(ECS::EntityID entityID, size_t index, float thumbnailSize);
        bool HasMetadata(const std::string& filePath);
        std::string GetFileDate(const std::string& filePath);
        std::string FormatDate(const std::filesystem::file_time_type& ftime);
    };

} // namespace GUI

#endif