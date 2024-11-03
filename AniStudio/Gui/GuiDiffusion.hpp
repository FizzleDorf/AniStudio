#pragma once
#include "../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"

using namespace ECS;

class GuiDiffusion {
public:       
    ~GuiDiffusion() {
        delete cfgComp;
        delete latentComp;
        delete samplerComp;

        if (modelComp)
            delete modelComp;
        if (ckptComp)
            delete ckptComp;
        if (loraComp)
            delete loraComp;
        if (clipLComp)
            delete clipLComp;
        if (clipGComp)
            delete clipGComp;
        if (t5xxlComp)
            delete t5xxlComp;  
        if (promptComp)
            delete promptComp;
    }

    void StartGui();
    void RenderCKPTLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void RenderQueue();
    void Render();

    void HandleQueueEvent();

    void UpdateBuffer(const std::string &source, char *buffer, size_t buffer_size) {
        strncpy(buffer, source.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }

private:
    std::mutex guiMutex;

    // Variables to handle the parameters for diffusion
    ModelComponent *modelComp = nullptr;
    CLipLComponent *clipLComp = nullptr;
    CLipGComponent *clipGComp = nullptr;
    T5XXLComponent *t5xxlComp = nullptr;
    DiffusionModelComponent *ckptComp = nullptr;
    LatentComponent *latentComp = nullptr;
    LoraComponent *loraComp = nullptr;
    PromptComponent *promptComp = nullptr;
    SamplerComponent *samplerComp = nullptr;
    CFGComponent *cfgComp = nullptr;
    ImageComponent *imageComp = nullptr;
    
    // ECS-related variables
    EntityManager &mgr = ECS::EntityManager::Ref();

    EntityID entity;
    EntityID i2IEntity;
};
