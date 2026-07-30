// ImageView.hpp
#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "GUI.h"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "ClipboardUtilities.hpp"
#include <pch.h>

namespace GUI {

    class ImageView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Image View",
            "category": "Viewers",
            "description": "A simple image viewer with copy functionality"
        })";
        }

        ImageView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm),
            selectedEntityID(0),
            imgIndex(0),
            showHistory(true),
            autoSwitchOnLoad(true),
            zoom(1.0f),
            offsetX(0.0f),
            offsetY(0.0f),
            lastEntityCount(0) {
            viewName = "ImageView";
        }
        ~ImageView() = default;

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
        void RenderImageContextMenu(ECS::EntityID entityID);

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