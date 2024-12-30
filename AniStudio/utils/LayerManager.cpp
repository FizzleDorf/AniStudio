#include "LayerManager.hpp"

LayerManager::LayerManager() {}

LayerManager::~LayerManager() {
    for (GLuint texture : layers) {
        glDeleteTextures(1, &texture);
    }
}

void LayerManager::AddLayer() {
    EntityID entityID = mgr.AddNewEntity();
    mgr.AddComponent<ImageComponent>(entityID);
    auto &imageComp = mgr.GetComponent<ImageComponent>(entityID); 
    InitLayer(imageComp.textureID);
    layers.push_back(imageComp.textureID);
}

GLuint LayerManager::GetLayerTexture(int index) const {
    return (index >= 0 && index < layers.size()) ? layers[index] : 0;
}

size_t LayerManager::GetLayerCount() const { return layers.size(); }

void LayerManager::InitLayer(GLuint &texture) {
    glGenTextures(1, &texture);
    if (texture == 0) {
        // Handle error if texture creation fails
        throw std::runtime_error("Failed to generate OpenGL texture");
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void LayerManager::SetCanvasSize(int newWidth, int newHeight) {
    if (width == newWidth && height == newHeight)
        return;

    width = newWidth;
    height = newHeight;

    for (GLuint &texture : layers) {
        glDeleteTextures(1, &texture);
        InitLayer(texture);
    }
}

void LayerManager::RemoveLayer(int index) {
    if (index >= 0 && index < layers.size()) {
        glDeleteTextures(1, &layers[index]);
        layers.erase(layers.begin() + index);
    }
}
