#pragma once

#include "GUI.h"
#include "EntityManager.hpp"
#include "SDCPPComponents.h"
#include "ContextMenuUtils.hpp"
#include <memory>
#include <unordered_map>
#include <functional>

namespace GUI {

	class DiffusionView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Image Diffusion",
            "category": "Diffusion",
            "description": "AI image generation with dynamic component organization"
        })";
		}

		DiffusionView(ECS::EntityManager& entityMgr);
		~DiffusionView();

		void Init() override;
		void Render() override;

		// Serialization methods
		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);

	private:
		// Entity IDs for different modes
		ECS::EntityID txt2imgEntity = 0;
		ECS::EntityID img2imgEntity = 0;
		ECS::EntityID editEntity = 0;

		// UI state
		int currentMode = 0; // 0=txt2img, 1=img2img, 2=edit
		int numQueues = 1;
		bool isPaused = false;

		// Component visibility tracking
		mutable std::unordered_map<std::string, bool> componentVisibility;

		// Context menu utilities
		std::unique_ptr<Utils::ContextMenuUtils> contextMenuUtils;

		// Entity management
		void ResetEntities();
		ECS::EntityID CreateEntityWithComponents(bool includeInputImage);
		bool IsEntitySafeToUse(ECS::EntityID entity) const;

		// Dynamic component rendering using compCategory
		void RenderEntityComponents(const ECS::EntityID entity);
		void RenderComponent(ECS::EntityID entity, ECS::ComponentTypeID compId, const std::string& componentName);

		// Context menus
		void RenderMainContextMenu();

		// Queue rendering
		void RenderQueueList();

		// Metadata controls
		void RenderMetadataControls();

		// Event handlers
		void HandleT2IEvent();
		void HandleI2IEvent();
		void HandleEditEvent();

		// Utility methods
		ECS::EntityID GetCurrentEntity() const;
		std::vector<std::string> GetCategoryRenderOrder() const;

		// Metadata methods
		void SaveMetadataToJson(const std::string& filepath);
		void LoadMetadataFromJson(const std::string& filepath);
		void LoadMetadataFromPNG(const std::string& imagePath);
	};

} // namespace GUI