#ifndef IMAGEVIEW_HPP
#define IMAGEVIEW_HPP

#include "ImageComponent.hpp"
#include <GL/glew.h>
#include <string>

namespace ECS {

class ImageView {
public:
    ImageView();
    void SetImageComponent(ImageComponent *imageComponent);
    void Render();
    void LoadImage(const std::string &filePath);
    void SaveImage(const std::string &filePath);
    ~ImageView();

private:
    void CreateTexture();

    ImageComponent *imageComponent;
    GLuint textureID;
};

} // namespace ECS

#endif
