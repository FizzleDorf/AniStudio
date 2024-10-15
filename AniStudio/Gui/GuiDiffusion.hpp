#pragma once

#include "SDCPPComponents.h"
#include "InferenceQueue.hpp"
#include "pch.h"
#include "ImageComponent.hpp"

using namespace ECS;

class GuiDiffusion {
public:       
    ~GuiDiffusion() {
        delete cfgComp;
        delete latentComp;
        delete samplerComp;

        if (ckptComp)
            delete ckptComp;
        if (loraComp)
            delete loraComp;
        if (promptComp)
            delete promptComp;
    }
    void SetECS(EntityManager *newMgr) { mgr = newMgr; }
    void StartGui();
    void RenderCKPTLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void Queue();
    void Render();

    void UpdateBuffer(const std::string &source, char *buffer, size_t buffer_size) {
        strncpy(buffer, source.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }

private:
    // Variables to handle the parameters for diffusion
    CFGComponent *cfgComp = nullptr;
    DiffusionModelComponent *ckptComp = nullptr;
    LatentComponent *latentComp = nullptr;
    LoraComponent *loraComp = nullptr;
    PromptComponent *promptComp = nullptr;
    SamplerComponent *samplerComp = nullptr;

    // ECS-related variables
    EntityManager *mgr = nullptr;
    EntityID t2IEntity;
    EntityID i2IEntity;
};
