/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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
        void ResetEntities();

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
		void HandleI2IEvent();
        void HandleUpscaleEvent();
        void SaveMetadataToJson(const std::string& filepath);
        void LoadMetadataFromJson(const std::string& filepath);
        void LoadMetadataFromPNG(const std::string& imagePath);
        void RenderMetadataControls();

    private:
        // Tab state
        bool isFilenameChanged = true;
        bool isPaused = false;
        int numQueues = 1;
        bool isTxt2ImgMode = true; // Tracks which tab is active
		
        // Entity IDs for the two modes
        EntityID txt2imgEntity = 0;
        EntityID img2imgEntity = 0;

		char fileName[256] = "AniStudio"; // Buffer for file name
		char outputDir[256] = "";         // Buffer for output directory

		int currentExtensionIndex = 0;
		std::string currentExtension = ".png"; // Default extension
    };

} // namespace GUI