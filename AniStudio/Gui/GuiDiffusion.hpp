#pragma once
#include "../Engine/Engine.hpp"
#include "ECS.h"
#include "pch.h"

using namespace ECS;

class GuiDiffusion {
public:    
    GuiDiffusion() {}
    ~GuiDiffusion() {}

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
    // ECS-related variables
    EntityManager &mgr = ECS::EntityManager::Ref();
    EntityID entity;

    std::mutex guiMutex;
    std::mutex modelPathMutex; // Mutex for synchronizing access to modelPath

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
    ImageComponent imageComp;
};