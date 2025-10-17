#pragma once

#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include "pch.h"
#include <components.h>
#include "SDCPPComponents.h" 
#include <memory>
#include <map>

// Forward declarations
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
            "category": "Tools",
            "description": "Upscale images with ESRGAN models using schema-driven UI."
        })";
		}

		UpscaleView(EntityManager &entityMgr, ImGuiContext* mainContext = nullptr);
		~UpscaleView();

		// BaseView override functions
		void Init() override;
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json &j) override;
		void Render() override;

	private:
		// Entity for upscaling template (like DiffusionView pattern)
		EntityID upscaleEntity = 0;

		// Component visibility tracking
		std::map<std::string, bool> componentVisibility;

		// Context menu utilities
		std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

		// Legacy state variables (to be removed if not needed)
		bool isFilenameChanged = false;

		// Core methods following DiffusionView pattern
		void ResetEntity();
		bool IsEntitySafeToUse(ECS::EntityID entity) const;
		std::vector<std::string> GetCategoryRenderOrder() const;
		void RenderQueueControls();

		// Component rendering
		void RenderEntityComponents(const EntityID entity);
		void RenderComponent(EntityID entity, ComponentTypeID compId, const std::string& componentName);
		void RenderMainContextMenu();

		// Event handlers
		void HandleUpscaleEvent();

		// Metadata functions
		void RenderMetadataControls();
		void SaveMetadataToJson(const std::string &filepath);
		void LoadMetadataFromJson(const std::string &filepath);

		// Queue control variables
		int numQueues = 1;
		bool isPaused = false;
	};

} // namespace GUI