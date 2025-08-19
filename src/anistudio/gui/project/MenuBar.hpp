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
#include "NewProjectView.hpp"
#include "LoadProjectView.hpp"
#include "ViewTypes.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace ANI {
	class ProjectManager;
	class Events;
}

namespace GUI {
	class ViewManager;

	// Structure for building hierarchical menus
	struct MenuNode {
		std::unordered_map<std::string, std::unique_ptr<MenuNode>> children;
		std::vector<std::pair<std::string, std::string>> views; // viewType, displayName pairs
	};

	class MenuBar {
	public:
		MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr);

		void Update(float deltaTime);
		void Render();

	private:
		ANI::ProjectManager& projectManager;
		ViewManager& viewManager;
		ANI::Events& events;

		// Direct view instances for dialogs
		NewProjectView newProjectView;
		LoadProjectView loadProjectView;

		// State for showing dialogs
		bool showNewProjectDialog = false;
		bool showLoadProjectDialog = false;

		// Workspace dialog state
		bool showCreateWorkspaceDialog = false;
		bool showRenameWorkspaceDialog = false;
		char createWorkspaceBuffer[256] = "New Workspace";
		char renameWorkspaceBuffer[256] = "";

		// Menu sections
		void ShowFileMenu();
		void ShowEditMenu();
		void ShowViewMenus();
		void ShowWorkspaceMenu();
		void ShowHelpMenu();

		// Workspace dialog rendering
		void RenderWorkspaceDialogs();

		// Hierarchical menu building
		std::vector<std::string> SplitCategoryPath(const std::string& category);
		void RenderMenuNode(const MenuNode& node);

		// Workspace helpers
		void CreateNewWorkspace();
		void DeleteCurrentWorkspace();
		bool IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const;
		void ToggleViewInCurrentWorkspace(const std::string& viewTypeName);
	};

} // namespace GUI