#pragma once
#include "Base/BaseView.hpp"
#include "pch.h"
#include <components.h>
#include <SDcppSystem.hpp>
#include "ImGuiFileDialog.h"
#include "stable-diffusion.h"
using namespace ECS;

struct ProgressData {
    std::atomic<int> currentStep{ 0 };
    std::atomic<int> totalSteps{ 0 };
    std::atomic<float> currentTime{ 0.0f };
    std::atomic<bool> isProcessing{ false };
};

namespace GUI {

    class DiffusionView : public BaseView {
    public:
        DiffusionView(EntityManager& entityMgr);
        ~DiffusionView();

        // Overloaded Functions
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void Render() override;
        void Init() override;

        // Generate both Txt2Img and Img2Img entities 
        void CreateEntities();

        // Gui Elements
        void RenderModelLoader(const EntityID entity);
        void RenderLatents(const EntityID entity);
        void RenderInputImage(const EntityID entity);
        void RenderSampler(const EntityID entity);
        void RenderPrompts(const EntityID entity);
        void RenderOther(const EntityID entity);
        void RenderQueueList();
        void RenderFilePath(const EntityID entity);
        void RenderDiffusionModelLoader(const EntityID entity);
        void RenderVaeLoader(const EntityID entity);
        void RenderControlnets(const EntityID entity);
        void RenderEmbeddings(const EntityID entity);
        void RenderVaeOptions(const EntityID entity);
        void HandleT2IEvent();
        void HandleUpscaleEvent();
        void SaveMetadataToJson(const std::string& filepath);
        void LoadMetadataFromJson(const std::string& filepath);
        void LoadMetadataFromPNG(const std::string& imagePath);
        void RenderMetadataControls();

    private:
        // Tab state
        bool isFilenameChanged = false;
        bool isPaused = false;
        int numQueues = 1;
        bool isTxt2ImgMode = true; // Tracks which tab is active

        // Entity IDs for the two modes
        EntityID txt2imgEntity = 0;
        EntityID img2imgEntity = 0;
    };

} // namespace GUI