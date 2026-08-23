#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "BaseMediaView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"

namespace GUI {

    class ImageView : public BaseMediaView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Image View",
            "category": "Viewers",
            "description": "A simple image viewer"
        })";
        }

        ImageView(ECS::EntityManager& mgr, ViewManager& vm);
        ~ImageView() = default;

        void Init() override;
        void Update(float deltaT) override;
        void Render() override;

        void LoadMedia(const std::vector<std::string>& filePaths) override;
        void SaveSelectedMedia() override;
        void SaveSelectedMediaAs(const std::string& filePath) override;
        void RemoveSelectedMedia() override;
        void RefreshEntities() override;

        void SetSelectedEntity(ECS::EntityID entity) override;

        bool IsHistoryVisible() const override;

        std::string GetSelectedFilePath() const override;

    protected:
        std::shared_ptr<ECS::ImageSystem> imageSystem;
        bool autoSwitchOnLoad;

        void OnMediaAdded(ECS::EntityID entity) override;
        void OnMediaRemoved(ECS::EntityID entity) override;
        std::string GetHistoryViewTypeName() const override;

        bool IsImageComponentOnly(ECS::EntityID entityId) const;

        void RenderMenuBar();
        void RenderImageInfo();
        void RenderControls();
        void RenderSelector();
        void RenderSelected();
    };

}
#endif