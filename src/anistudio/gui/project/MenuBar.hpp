#pragma once

#include "ProjectPopups.hpp"
#include "ViewTypes.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <set>

namespace ANI {
	class ProjectManager;
	class Events;
}

namespace GUI {
	class ViewManager;

	struct MenuNode {
		std::unordered_map<std::string, std::unique_ptr<MenuNode>> children;
		std::vector<std::pair<std::string, std::string>> views; // viewTypeName, displayName
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

		// Simple popup state
		ProjectPopupState popupState;

		// Workspace dialogs
		bool showCreateWorkspaceDialog = false;
		bool showRenameWorkspaceDialog = false;
		char createWorkspaceBuffer[256] = "New Workspace";
		char renameWorkspaceBuffer[256] = "";

		// Menu rendering methods
		void ShowFileMenu();
		void ShowEditMenu();
		void ShowWorkspaceMenu();
		void ShowCustomCategoryMenus();  // Renders category-based custom menus
		void ShowHelpMenu();

		// Workspace dialog rendering
		void RenderWorkspaceDialogs();

		// Category-based menu helpers
		void RenderViewsForCategory(const std::string& categoryName);
		std::vector<std::string> GetCustomTopLevelCategories() const;

		// Hierarchical menu helpers
		std::vector<std::string> SplitCategoryPath(const std::string& category) const;
		void RenderMenuNode(const MenuNode& node);

		// Workspace management
		void CreateNewWorkspace();
		void DeleteCurrentWorkspace();
		bool IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const;
		void ToggleViewInCurrentWorkspace(const std::string& viewTypeName);
	};

} // namespace GUI