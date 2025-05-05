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

namespace GUI {

	class UpscaleView : public BaseView {
	public:
		UpscaleView(EntityManager &entityMgr);
		~UpscaleView();

		// BaseView override functions
		void Init() override;
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json &j) override;
		void Render() override;

		// Render UI components
		void RenderInputImageSelector(const EntityID entity);
		void RenderModelSelector(const EntityID entity);
		void RenderUpscaleParams(const EntityID entity);
		void RenderOutputSettings(const EntityID entity);
		void RenderThreadsSettings(const EntityID entity);

		// Event handlers
		void HandleUpscaleEvent();
		void ResetEntity();

		// Metadata functions
		void SaveMetadataToJson(const std::string &filepath);
		void LoadMetadataFromJson(const std::string &filepath);
		void RenderMetadataControls();

	private:
		// Entity for upscaling operations
		EntityID upscaleEntity = 0;
		char outFileName[256] = "Anistudio-Upscale";
		char outDirPath[512];
		int currentExtensionIndex = 0;
		std::string currentExtension = ".png"; // Default extension
		// State variables
		bool isFilenameChanged = false;
	};

} // namespace GUI