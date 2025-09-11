#pragma once

#include "ProjectPopups.hpp"
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

	struct MenuNode {
		std::unordered_map<std::string, std::unique_ptr<MenuNode>> children;
		std::vector<std::pair<std::string, std::string>> views;
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

		// Simple popup state - no more separate classes!
		ProjectPopupState popupState;

		// Workspace dialogs
		bool showCreateWorkspaceDialog = false;
		bool showRenameWorkspaceDialog = false;
		char createWorkspaceBuffer[256] = "New Workspace";
		char renameWorkspaceBuffer[256] = "";

		void ShowFileMenu();
		void ShowEditMenu();
		void ShowViewMenus();
		void ShowWorkspaceMenu();
		void ShowHelpMenu();

		void RenderWorkspaceDialogs();

		std::vector<std::string> SplitCategoryPath(const std::string& category);
		void RenderMenuNode(const MenuNode& node);

		void CreateNewWorkspace();
		void DeleteCurrentWorkspace();
		bool IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const;
		void ToggleViewInCurrentWorkspace(const std::string& viewTypeName);
	};

} // namespace GUI