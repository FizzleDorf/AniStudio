#pragma once

#include "SDCPPComponents.h"
#include "pch.h"

using namespace ECS;

class GuiDiffusion {
public:
    GuiDiffusion()
        : cfg(nullptr), modelPath(nullptr), latentWidth(nullptr), latentHeight(nullptr), strength(nullptr),
          clipStrength(nullptr), loraReference(nullptr), posPrompt(nullptr), negPrompt(nullptr), samplerSteps(nullptr),
          scheduler(nullptr), sampler(nullptr), denoise(nullptr) {}

    ~GuiDiffusion() {
        delete cfg;
        delete latentWidth;
        delete latentHeight;
        delete strength;
        delete clipStrength;
        delete denoise;
        delete samplerSteps;

        if (modelPath)
            delete modelPath;
        if (loraReference)
            delete loraReference;
        if (posPrompt)
            delete posPrompt;
        if (negPrompt)
            delete negPrompt;
        if (scheduler)
            delete scheduler;
        if (sampler)
            delete sampler;
    }

    void SetECS(EntityManager *newMgr) { mgr = newMgr; }
    void StartGui();
    void RenderCKPTLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void RenderCommands();
    void Render();

    void UpdateBuffer(const std::string &source, char *buffer, size_t buffer_size) {
        strncpy(buffer, source.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }

private:
    // Variables to handle the parameters for diffusion
    float *cfg;
    std::string *modelPath;
    int *latentWidth;
    int *latentHeight;
    float *strength;
    float *clipStrength;
    std::string *loraReference;
    std::string *posPrompt;
    std::string *negPrompt;
    int *samplerSteps;
    std::string *scheduler;
    std::string *sampler;
    float *denoise;

    // ECS-related variables
    EntityManager *mgr = nullptr;
    EntityID t2IEntity;
    EntityID i2IEntity;
};
