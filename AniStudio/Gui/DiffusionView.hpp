#pragma once
#include "BaseView.hpp"
#include "Control.hpp"
#include "pch.h"

using namespace ECS;

class DiffusionView : public BaseView {
public:
    DiffusionView() { seedControl = new Control<int>(samplerComp.seed, ControlMode::Fixed); }
    ~DiffusionView() {
        if (seedControl)
            delete seedControl;
    }

    void RenderModelLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void RenderOther();
    void RenderQueueList();
    void RenderFilePath();
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
    bool isFilenameChanged = false;
    int numQueues = 1;
    // ECS-related variables
    EntityID entity;
    Control<int> *seedControl = nullptr;
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