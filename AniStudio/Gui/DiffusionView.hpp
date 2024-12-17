#pragma once
#include "BaseView.hpp"
#include "pch.h"

using namespace ECS;

class DiffusionView : public BaseView {
public:    
    DiffusionView() {}
    ~DiffusionView() {}

    void RenderModelLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void RenderQueue();
    void Render();

    void RenderDiffusionModelLoader();
    void RenderVaeLoader();
    void RenderControlnets();
    void RenderEmbeddings();

    void HandleT2IEvent();
    void HandleUpscaleEvent();

    void PrepareNewEntity();
    void CopyNewEntity(const EntityID oldEntity);

    void UpdateBuffer(const std::string &source, char *buffer, size_t buffer_size) {
        strncpy(buffer, source.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }

private:
    // ECS-related variables
    EntityID entity;

    // Variables to handle the parameters for diffusion
    ModelComponent modelComp;
    CLipLComponent clipLComp;
    CLipGComponent clipGComp;
    T5XXLComponent t5xxlComp;
    DiffusionModelComponent ckptComp;
    LatentComponent latentComp;
    LoraComponent loraComp;
    PromptComponent promptComp;
    SamplerComponent samplerComp;
    CFGComponent cfgComp;
    VaeComponent vaeComp;
    TaesdComponent taesdComp;
    ImageComponent imageComp;
    EmbeddingComponent embedComp;
    ControlnetComponent controlComp;
    LayerSkipComponent layerSkipComp;
};