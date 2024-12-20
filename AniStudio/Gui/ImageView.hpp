#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "BaseView.hpp"
#include "ImageComponent.hpp"
#include "LoadedHeaps.hpp"
#include <pch.h>

namespace ECS {

class ImageView : public BaseView {
public:
    ImageView();
    void Render();
    void LoadImage();
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture();
    void CleanUpCurrentImage();
    void RenderSelector();

    int imgIndex = 0;
    ImageComponent imageComponent;
};

} // namespace ECS

#endif
