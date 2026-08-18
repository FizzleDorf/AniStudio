#ifndef IMAGEHISTORYVIEW_HPP
#define IMAGEHISTORYVIEW_HPP

#include "BaseView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"

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

        void RefreshEntities();
        void OnImageAdded(ECS::EntityID entity);
        void OnImageRemoved(ECS::EntityID entity);
        void SelectImage(ECS::EntityID entity);
    };

} // namespace GUI

#endif