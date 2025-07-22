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
 */

#pragma once
#include "BaseView.hpp"
#include "ViewTypes.hpp"
#include <unordered_map>
#include <memory>

namespace ANI { class ProjectManager; }

namespace GUI {
	class ViewManager;

	// Structure for building hierarchical menus
	struct MenuNode {
		std::unordered_map<std::string, std::unique_ptr<MenuNode>> children;
		std::vector<std::pair<std::string, ViewMetadata>> views;
	};

	class MenuBar : public BaseView {
	public:
		MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr, ECS::EntityManager& entityMgr);

		static constexpr const char* GetMetadataJSON() {
			return R"({
				"displayName": "Menu Bar",
				"category": "Hidden",
				"description": "Main application menu bar"
			})";
		}

		void Init() override;
		void Update(const float deltaT) override;
		void Render() override;

		// Set the workspace that this menubar manages
		void SetManagedWorkspace(WorkspaceID workspaceId) { m_managedWorkspace = workspaceId; }
		WorkspaceID GetManagedWorkspace() const { return m_managedWorkspace; }

	private:
		ANI::ProjectManager& m_projectManager;
		ECS::EntityManager& m_entityManager;
		ViewManager& m_viewManager;

		// The workspace this menubar manages
		WorkspaceID m_managedWorkspace = 0;

		// Track actual view instances created by this menubar
		std::unordered_map<std::string, WorkspaceID> m_activeViewInstances;

		// Menu sections
		void ShowFileMenu();
		void ShowEditMenu();
		void ShowCategoryMenus();
		void ShowHelpMenu();

		// Hierarchical menu building
		std::vector<std::string> SplitCategoryPath(const std::string& category);
		void RenderMenuNode(const MenuNode& node);

		// View instance management (creates/destroys actual view instances)
		void SyncViewState();
		bool IsViewInstanceInWorkspace(const std::string& viewTypeName) const;
		void ToggleViewInstanceInWorkspace(const std::string& viewType);

		// Helper methods for managing actual view instances
		void CreateViewInstance(const std::string& viewTypeName);
		void RemoveViewInstance(const std::string& viewTypeName);
		bool CheckViewExistsByName(const std::string& viewTypeName) const;
	};

} // namespace GUI