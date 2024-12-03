#ifndef LAYER_MANAGER_HPP
#define LAYER_MANAGER_HPP

#include <GL/glew.h>
#include <vector>

class LayerManager {
public:
    LayerManager(int canvasWidth, int canvasHeight);
    ~LayerManager();

    void AddLayer();
    GLuint GetLayerTexture(int index) const;
    size_t GetLayerCount() const;

private:
    int width, height;
    std::vector<GLuint> layers;

    void InitLayer(GLuint &texture);
};

#endif // LAYER_MANAGER_HPP
