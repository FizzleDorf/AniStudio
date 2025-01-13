#ifndef LAYERMANAGER_HPP
#define LAYERMANAGER_HPP

#include "pch.h"
#include <ECS.h>
#include <components.h>

using namespace ECS;
namespace GUI {
class LayerManager {
public:
    LayerManager(EntityManager &entityMgr);

    ~LayerManager();

    void AddLayer();
    void RemoveLayer(int index);
    GLuint GetLayerTexture(int index) const;
    size_t GetLayerCount() const;

    // Set the canvas size (resizes all layers)
    void SetCanvasSize(int newWidth, int newHeight);

private:
    // Initializes a new OpenGL texture for a layer
    void InitLayer(GLuint &texture);
    std::vector<GLuint> layers;

    // Canvas dimensions
    int width = 800;  // Default width
    int height = 600; // Default height
    EntityManager &mgr;
};
} // namespace GUI

#endif // LAYERMANAGER_HPP