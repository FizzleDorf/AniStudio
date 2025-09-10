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