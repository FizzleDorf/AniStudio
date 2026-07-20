// ImageView.hpp
#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "GUI.h"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "ContextMenuUtils.hpp"
#include <pch.h>

namespace GUI {

    class ImageView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Image View",
            "category": "Viewers",
            "description": "A simple image viewer with copy/paste functionality"
        })";
        }

        ImageView(ECS::EntityManager& entityMgr);
        ~ImageView();

        void Init() override;
        void Update(const float deltaT) override;
        void Render() override;

    private:
        ECS::EntityID selectedEntityID;
        int imgIndex;
        bool showHistory;
        bool autoSwitchOnLoad;
        size_t lastEntityCount;

        float zoom;
        float offsetX;
        float offsetY;

        std::vector<ECS::EntityID> imageEntities;

        std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

        std::shared_ptr<ECS::ImageSystem> imageSystem;

        void OnImageLoaded(ECS::EntityID entityID);
        void OnImageRemoved(ECS::EntityID entityID);
        void RefreshImageEntities();

        void RenderMenuBar();
        void RenderImageInfo();
        void RenderControls();
        void RenderSelector();
        void RenderHistory();
        void RenderSelectedImage();
        void DrawGrid(int imageWidth, int imageHeight);

        void SetZoom(float newZoom);
        void LoadImages(const std::vector<std::string>& filePaths);
        void SaveSelectedImage();
        void SaveSelectedImageAs(const std::string& filePath);
        void RemoveSelectedImage();

        std::string TruncateFilename(const std::string& filename, float maxTextWidth);

        bool IsImageComponentOnly(ECS::EntityID entityId) const;
    };

} // namespace GUI

#endif // IMAGEVIEW_HPP