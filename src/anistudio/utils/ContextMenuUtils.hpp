#pragma once

#include "EntityManager.hpp"
#include "Types.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <string>
#include <vector>
#include <map>

namespace Utils {

	class ContextMenuUtils {
	public:
		ContextMenuUtils(ECS::EntityManager& entityMgr);
		~ContextMenuUtils() = default;

		// Main context menu rendering
		void RenderImageContextMenu(ECS::EntityID entityId);
		void RenderImageContextMenuWithPath(const std::string& imagePath);
		void RenderComponentContextMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
		void RenderComponentContextMenu(ECS::EntityID entityId, const std::string& componentName);

		// Copy operations
		void CopyEntity(ECS::EntityID entityId);
		void CopyComponent(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
		void CopyComponent(ECS::EntityID entityId, const std::string& componentName);

		// Paste operations
		bool PasteEntity(ECS::EntityID targetEntityId);
		bool PasteComponent(ECS::EntityID targetEntityId, const std::string& componentName);
		bool PasteValue(ECS::EntityID targetEntityId, const std::string& componentName, const std::string& propertyName);

		// Clipboard utilities
		bool HasClipboardEntity() const;
		std::string GetClipboardPreview() const;
		void ClearClipboard();

		// Helper functions
		std::vector<std::string> GetClipboardComponents() const;
		std::vector<std::string> GetCommonProperties(ECS::EntityID targetEntityId, const std::string& componentName) const;

		// Public method to get clipboard data
		nlohmann::json GetClipboardData() const;
		void RenderPasteMenu(ECS::EntityID entityId); 

	private:
		ECS::EntityManager& entityManager;

		// Image metadata parsing
		nlohmann::json ParseImageMetadata(const std::string& imagePath);
		std::vector<std::string> ExtractComponentsFromMetadata(const nlohmann::json& metadata);

		// Context menu rendering helpers
		void RenderMetadataComponentMenu(const nlohmann::json& metadata);
		void RenderEntityComponentMenu(ECS::EntityID entityId);
		// REMOVED: void RenderPasteMenu(ECS::EntityID entityId);  <-- REMOVE THIS FROM PRIVATE
		void RenderValueCopyMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId, const std::string& componentName);
		void RenderMetadataValueMenu(const nlohmann::json& metadata);

		// Component operations from metadata
		void CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName);
		void CopyEntityFromMetadata(const nlohmann::json& metadata);

		// Clipboard operations
		void SetClipboardData(const nlohmann::json& data);

		// Value extraction
		std::string GetClipboardValue(const std::string& componentName, const std::string& propertyName) const;
		nlohmann::json GetClipboardComponent(const std::string& componentName) const;
	};

}