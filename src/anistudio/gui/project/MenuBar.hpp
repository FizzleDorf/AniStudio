#pragma once
#include "ProjectPopups.hpp"
#include "AniStudio.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set>

namespace ANI {
    class ProjectSystem;
    class StudioCore;
}

namespace GUI {
    class ViewManager;

    class MenuBar {
    public:
        MenuBar(ANI::ProjectSystem& projectSystem, ViewManager& viewMgr, ANI::StudioCore& m_studioCore);

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
            std::vector<std::pair<std::string, std::string>> views;
        };
        void RenderMenuNode(const MenuNode& node);

        // Workspace operations
        void RenderWorkspaceDialogs();
        void CreateNewWorkspace();
        void DeleteCurrentWorkspace();

        // View operations
        bool IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const;
        void ToggleViewInCurrentWorkspace(const std::string& viewTypeName);

        // References
        ANI::ProjectSystem& projectSystem;
        ViewManager& viewManager;
        ANI::StudioCore& m_studioCore;

        // Popup state
        ProjectPopupState popupState;

        // Dialog state
        bool showCreateWorkspaceDialog = false;
        bool showRenameWorkspaceDialog = false;
        char createWorkspaceBuffer[256] = "New Workspace";
        char renameWorkspaceBuffer[256] = "";
    };

} // namespace GUI