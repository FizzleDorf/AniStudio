#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "BaseView.hpp"
#include "ImageComponent.hpp"
#include <pch.h>

namespace ECS {

class ImageView : public BaseView {
public:
    ImageView();
    void SetCurrentEntity(const EntityID newEntity) { entity = newEntity; }
    void Render();
    void LoadImage();
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture();
    void SelectImage();

    EntityID entity = NULL;
    ImageComponent imageComponent;
};

} // namespace ECS

#endif
