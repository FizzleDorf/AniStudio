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