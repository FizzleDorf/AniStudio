#pragma once

#include "ViewManager.hpp"
#include "EntityManager.hpp"
#include "SDCPPComponents.h"
#include "Components.h"
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

		DiffusionView::DiffusionView(ECS::EntityManager& mgr, ViewManager& vm)
			: BaseView(mgr, vm)
		{
			viewName = "DiffusionView";
			windowOpen = true;
			contextMenuUtils = std::make_unique<Utils::ContextMenuUtils>(m_entityManager);
		}
		
		~DiffusionView() {
			QuickSave();
			if (txt2imgEntity != 0) m_entityManager.DestroyEntity(txt2imgEntity);
			if (img2imgEntity != 0) m_entityManager.DestroyEntity(img2imgEntity);
			if (editEntity != 0) m_entityManager.DestroyEntity(editEntity);
		}

		ECS::EntityID GetCurrentEntity() const;
		int GetCurrentMode() const { return currentMode; }

		void Init() override;
		void Render() override;

		// Serialization methods
		nlohmann::json Serialize() const override;
		void Deserialize(const nlohmann::json& j) override;

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

		// Menu bar
		void RenderMenuBar();

		// Queue rendering
		void RenderQueueList();

		// Event handlers
		void HandleT2IEvent();
		void HandleI2IEvent();
		void HandleEditEvent();
		
		std::vector<std::string> GetCategoryRenderOrder() const;

		// Component management
		void ToggleComponent(ECS::EntityID entity, const std::string& name);
		bool IsComponentPresent(ECS::EntityID entity, const std::string& name) const;
		void SetModelMode(ECS::EntityID entity, bool useCheckpoint);
		bool IsCheckpointMode(ECS::EntityID entity) const;
		void ResetToDefaultComponents(ECS::EntityID entity);
		std::vector<std::string> GetAvailableComponentNames() const;

		// Metadata methods
		void SaveMetadataToJson(const std::string& filepath);
		void LoadMetadataFromJson(const std::string& filepath);
		void LoadMetadataFromPNG(const std::string& imagePath);
		void QuickSave();
		void QuickLoad();
		nlohmann::json SerializeAllEntities() const;
		void DeserializeAllEntities(const nlohmann::json& j);
	};

} // namespace GUI