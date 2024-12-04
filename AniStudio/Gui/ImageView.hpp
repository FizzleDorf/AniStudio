#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "ImageComponent.hpp"
#include "imgui.h"
#include "ECS.h"
#include "../backends/imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <string>

namespace ECS {

class ImageView {
public:
    ImageView();
    void SetCurrentEntity(const EntityID newEntity) { entity = newEntity; }
    void Render();
    void LoadImage(const std::string &filePath);
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture();

    EntityManager &mgr = ECS::EntityManager::Ref();
    EntityID entity = NULL;
    ImageComponent imageComponent;
};

} // namespace ECS

#endif
