#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "Base/BaseView.hpp"
#include "ImageComponent.hpp"
#include "LoadedMedia.hpp"
#include <pch.h>

namespace GUI {

class ImageView : public BaseView {
public:
    ImageView();
    void Render() override;
    void LoadImage();
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture(const int index);
    void CreateCurrentTexture();

    void CleanUpImage(const int index);
    void CleanUpCurrentImage();

    void RenderSelector();
    void RenderHistory();

    int imgIndex = 0;
    bool showHistory = false;
    ImageComponent imageComponent;
};

} // namespace ECS

#endif
