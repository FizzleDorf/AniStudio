#include "LayerManager.hpp"


LayerManager::LayerManager() {}

LayerManager::~LayerManager() {
    for (GLuint texture : layers) {
        glDeleteTextures(1, &texture);
    }
}

void LayerManager::AddLayer() {
    GLuint texture;
    InitLayer(texture);
    layers.push_back(texture);
}

GLuint LayerManager::GetLayerTexture(int index) const { return index < layers.size() ? layers[index] : 0; }

size_t LayerManager::GetLayerCount() const { return layers.size(); }

void LayerManager::InitLayer(GLuint &texture) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void LayerManager::SetCanvasSize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
}
