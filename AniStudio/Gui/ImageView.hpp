#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Base/BaseView.hpp"
#include "ImageComponent.hpp"
#include "ImageSystem.hpp"
#include "LoadedMedia.hpp"
#include <pch.h>

namespace GUI {

class ImageView : public BaseView {
public:
    ImageView(ECS::EntityManager &entityMgr) : BaseView(entityMgr), imgIndex(0), showHistory(false) {
        viewName = "ImageView";
        entityMgr.RegisterSystem<ECS::ImageSystem>();
    }
    void Render() override;
    void LoadImage();
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture(const int index);
    void CreateCurrentTexture();

    void CleanUpImage(const int index);
    void CleanUpCurrentImage();
    void DrawGrid();
    void RenderSelector();
    void RenderHistory();

    void SetZoom(float newZoom);

    int imgIndex = 0;
    bool showHistory = false;
    ImageComponent imageComponent;
};

} // namespace ECS

#endif
