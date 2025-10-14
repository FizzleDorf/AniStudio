#pragma once
#include "ProjectPopups.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>

namespace ANI {
	class ProjectManager;
}

namespace GUI {
	class ViewManager;

	class MenuBar {
	public:
		MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr);

		void Update(float deltaTime);
		void Render();

	private:
		// Menu rendering
		void ShowFileMenu();
		void ShowEditMenu();
		void ShowWorkspaceMenu();
		void ShowCustomCategoryMenus();
		void ShowHelpMenu();

		// View category handling
		void RenderViewsForCategory(const std::string& categoryName);
		std::vector<std::string> GetCustomTopLevelCategories() const;
		std::vector<std::string> SplitCategoryPath(const std::string& category) const;

		// Menu tree structure
		struct MenuNode {
			std::map<std::string, std::unique_ptr<MenuNode>> children;
			std::vector<std::pair<std::string, std::string>> views; // viewTypeName, displayName
		};
		void RenderMenuNode(const MenuNode& node);

		// Workspace operations
		void RenderWorkspaceDialogs();
		void CreateNewWorkspace();
		void DeleteCurrentWorkspace();

		// View operations
		bool IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const;
		void ToggleViewInCurrentWorkspace(const std::string& viewTypeName);

		// References - no initialization needed for references
		ANI::ProjectManager& projectManager;
		ViewManager& viewManager;

		// Popup state
		ProjectPopupState popupState;

		// Dialog state
		bool showCreateWorkspaceDialog = false;
		bool showRenameWorkspaceDialog = false;
		char createWorkspaceBuffer[256] = "New Workspace";
		char renameWorkspaceBuffer[256] = "";
	};

} // namespace GUI