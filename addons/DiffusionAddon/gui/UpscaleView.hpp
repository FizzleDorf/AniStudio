#pragma once

#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include "pch.h"
#include "Components.h"
#include "SDCPPComponents.h" 
#include "DiffusionCallbackUtils.hpp"
#include <memory>
#include <map>
#include <iomanip>
#include <sstream>

namespace Utils {
	class ContextMenuUtils;
}

using namespace ECS;

namespace GUI {

	class UpscaleView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Upscale View",
            "category": "Diffusion",
            "description": "Upscale images with ESRGAN models using schema-driven UI."
        })";
		}

		UpscaleView(EntityManager &entityMgr, ImGuiContext* mainContext = nullptr);
		~UpscaleView();

		void Init() override;
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json &j) override;
		void Render() override;

	private:
		EntityID upscaleEntity = 0;

		std::map<std::string, bool> componentVisibility;
		std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

		bool isFilenameChanged = false;

		void ResetEntity();
		bool IsEntitySafeToUse(ECS::EntityID entity) const;
		std::vector<std::string> GetCategoryRenderOrder() const;
		void RenderQueueList();

		void RenderEntityComponents(const EntityID entity);
		void RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& componentName);
		void RenderMainContextMenu();

		void HandleUpscaleEvent();

		// Metadata functions
		void RenderMetadataControls();
		void SaveMetadataToJson(const std::string &filepath);
		void LoadMetadataFromJson(const std::string &filepath);

		int numQueues = 1;
		bool isPaused = false;
	};

} // namespace GUI