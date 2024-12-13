#ifndef LAYER_MANAGER_HPP
#define LAYER_MANAGER_HPP

#include <GL/glew.h>
#include <vector>

class LayerManager {
public:
    LayerManager();
    ~LayerManager();

    void AddLayer();
    GLuint GetLayerTexture(int index) const;
    size_t GetLayerCount() const;

    void SetCanvasSize(int newWidth, int newHeight);

private:
    int width = 0; 
    int height = 0;
    std::vector<GLuint> layers;

    void InitLayer(GLuint &texture);
};

#endif // LAYER_MANAGER_HPP
