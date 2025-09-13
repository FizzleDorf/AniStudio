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
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include "EntityManager.hpp"
#include "Types.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <string>
#include <vector>

namespace Utils {

	class ContextMenuUtils {
	public:
		ContextMenuUtils(ECS::EntityManager& entityMgr);
		~ContextMenuUtils() = default;

		// Main context menu rendering for images with metadata
		void RenderImageContextMenu(ECS::EntityID entityId);
		void RenderImageContextMenuWithPath(const std::string& imagePath);

		// Legacy method for DiffusionView compatibility
		void RenderComponentContextMenu(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
		void RenderComponentContextMenu(ECS::EntityID entityId, const std::string& componentName);

		// Component operations from clipboard
		void CopyComponent(ECS::EntityID entityId, ECS::ComponentTypeID componentId);
		void CopyComponent(ECS::EntityID entityId, const std::string& componentName);
		bool PasteComponent(ECS::EntityID targetEntityId);
		bool PasteEntityToExisting(ECS::EntityID targetEntityId);
		bool CanPasteComponent() const;

		// Entity operations
		void CopyEntity(ECS::EntityID entityId);
		ECS::EntityID PasteEntity();
		bool CanPasteEntity() const;

		// Clipboard utilities
		bool HasValidClipboardData() const;
		std::string GetClipboardPreview() const;
		void SetClipboardData(const nlohmann::json& data);
		nlohmann::json GetClipboardData() const;
		void ClearClipboard();

	private:
		ECS::EntityManager& entityManager;

		// Image metadata parsing
		nlohmann::json ParseImageMetadata(const std::string& imagePath);
		std::vector<std::string> ExtractComponentsFromMetadata(const nlohmann::json& metadata);

		// Context menu rendering helpers
		void RenderMetadataComponentMenu(const nlohmann::json& metadata);
		void RenderEntityComponentMenu(ECS::EntityID entityId);
		void RenderPasteMenu(ECS::EntityID entityId);

		// Component operations from metadata
		void CopyComponentFromMetadata(const nlohmann::json& metadata, const std::string& componentName);
		void CopyEntityFromMetadata(const nlohmann::json& metadata);

		// Utility functions
		bool IsComponentRegistered(const std::string& componentName) const;
	};

} // namespace Utils