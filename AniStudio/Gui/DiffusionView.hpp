#pragma once
#include "Base/BaseView.hpp"
#include "pch.h"
#include <components.h>
#include <SDcppSystem.hpp>
#include "ImGuiFileDialog.h"
#include "stable-diffusion.h"
using namespace ECS;

struct ProgressData {
    std::atomic<int> currentStep{0};
    std::atomic<int> totalSteps{0};
    std::atomic<float> currentTime{0.0f};
    std::atomic<bool> isProcessing{false};
};

namespace GUI {

class DiffusionView : public BaseView {
public:
    DiffusionView(EntityManager &entityMgr);
    ~DiffusionView() {
        // if (seedControl)
        //     delete seedControl;
    }
    
    // Overloaded Functions
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json &j) override;
    void Render() override;
    // void ResetProgress();
    // Gui Elements
    void RenderModelLoader();
    void RenderLatents();
    void RenderInputImage();
    void RenderSampler();
    void RenderPrompts();
    void RenderOther();
    void RenderQueueList();
    void RenderFilePath();
    void RenderDiffusionModelLoader();
    void RenderVaeLoader();
    void RenderControlnets();
    void RenderEmbeddings();
    void RenderVaeOptions();
    void HandleT2IEvent();
    void HandleUpscaleEvent();
    void SaveMetadataToJson(const std::string &filepath);
    void LoadMetadataFromJson(const std::string &filepath);
    void LoadMetadataFromExif(const std::string &imagePath);
    void RenderMetadataControls();
    void LoadMetadataFromPNG(const std::string &imagePath);
    void UpdateBuffer(const std::string &source, char *buffer, size_t buffer_size) {
        strncpy(buffer, source.c_str(), buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }

private:
    bool isFilenameChanged = false;
    int numQueues = 1;
    // ECS-related variables
    EntityID entity;
    // Control<int> *seedControl = nullptr;
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
    VaeComponent vaeComp;
    TaesdComponent taesdComp;
    ImageComponent imageComp;
    EmbeddingComponent embedComp;
    ControlnetComponent controlComp;
    LayerSkipComponent layerSkipComp;
    GuidanceComponent guidanceComp;
    ClipSkipComponent clipSkipComp;
};
} // namespace GUI