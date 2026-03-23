#include "MenuBar.hpp"
#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>

namespace GUI {

    MenuBar::MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr, ANI::StudioCore& studioCore)
        : projectManager(projectMgr), viewManager(viewMgr), studioCore(studioCore) {

        popupState.InitializeBuffers(projectMgr);
        popupState.LoadTemplates();
        popupState.RefreshRecentProjects(projectMgr);
    }

    void MenuBar::Update(float deltaTime) {
        // Nothing to update now that we don't have separate view classes
    }

    void MenuBar::Render() {
        // Render project popups
        ProjectPopups::RenderNewProjectPopup(popupState, projectManager);
        ProjectPopups::RenderLoadProjectPopup(popupState, projectManager);

        // Settings view is rendered by StudioCore, not here

        if (ImGui::BeginMenuBar()) {
            ShowFileMenu();
            ShowEditMenu();
            ShowWorkspaceMenu();
            ShowCustomCategoryMenus();
            ShowHelpMenu();
            ImGui::EndMenuBar();
        }

        RenderWorkspaceDialogs();
    }

    void MenuBar::ShowFileMenu() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project...", "Ctrl+N")) {
                popupState.showNewProjectPopup = true;
            }

            if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
                popupState.showLoadProjectPopup = true;
                popupState.RefreshRecentProjects(projectManager);
            }

            ImGui::Separator();

            bool projectOpen = projectManager.IsProjectOpen();
            if (ImGui::MenuItem("Save Project", "Ctrl+S", false, projectOpen)) {
                ANI::Events::Ref().QueueEvent("SaveProject");
            }

            if (ImGui::MenuItem("Close Project", "", false, projectOpen)) {
                ANI::Events::Ref().QueueEvent("CloseProject");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Settings...", "Ctrl+,")) {
                studioCore.GetSettingsView().Show();
            }

            if (projectOpen) {
                RenderViewsForCategory("File");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                ANI::Events::Ref().QueueEvent("Quit");
            }

            ImGui::EndMenu();
        }
    }

    void MenuBar::ShowEditMenu() {
        if (ImGui::BeginMenu("Edit")) {
            // Render views that have "Edit" as their top-level category
            if (projectManager.IsProjectOpen()) {
                RenderViewsForCategory("Edit");
            }

            ImGui::Separator();
            ImGui::EndMenu();
        }
    }

    void MenuBar::ShowWorkspaceMenu() {
        if (!projectManager.IsProjectOpen()) return;

        if (ImGui::BeginMenu("Workspace")) {
            auto allWorkspaces = viewManager.GetAllWorkspaces();
            GUI::WorkspaceID currentActiveWorkspace = projectManager.GetViewState().GetLastActiveWorkspace();

            std::string activeName = viewManager.GetWorkspaceName(currentActiveWorkspace);
            ImGui::Text("Active: %s", activeName.c_str());
            ImGui::Separator();

            for (GUI::WorkspaceID workspaceID : allWorkspaces) {
                bool isActive = (workspaceID == currentActiveWorkspace);
                std::string workspaceName = viewManager.GetWorkspaceName(workspaceID);

                if (ImGui::MenuItem(workspaceName.c_str(), nullptr, isActive)) {
                    if (!isActive) {
                        ANI::Events::Ref().QueueEventWithData("SetActiveWorkspace", workspaceID);
                        std::cout << "[MenuBar] Queued SetActiveWorkspace for workspace: " << workspaceID << std::endl;
                    }
                }
            }

            // Render views that have "Workspace" as their top-level category
            RenderViewsForCategory("Workspace");

            ImGui::Separator();

            if (ImGui::MenuItem("New Workspace...")) {
                showCreateWorkspaceDialog = true;
            }

            if (ImGui::MenuItem("Rename Current Workspace...")) {
                showRenameWorkspaceDialog = true;
                std::string currentName = viewManager.GetWorkspaceName(currentActiveWorkspace);
                strncpy(renameWorkspaceBuffer, currentName.c_str(), sizeof(renameWorkspaceBuffer) - 1);
                renameWorkspaceBuffer[sizeof(renameWorkspaceBuffer) - 1] = '\0';
            }

            if (allWorkspaces.size() > 1) {
                if (ImGui::MenuItem("Delete Current Workspace")) {
                    DeleteCurrentWorkspace();
                }
            }

            ImGui::EndMenu();
        }
    }

    void MenuBar::ShowCustomCategoryMenus() {
        if (!projectManager.IsProjectOpen()) return;

        // Get all unique top-level categories from registered views
        auto customCategories = GetCustomTopLevelCategories();

        for (const auto& category : customCategories) {
            if (ImGui::BeginMenu(category.c_str())) {
                RenderViewsForCategory(category);
                ImGui::EndMenu();
            }
        }
    }

    void MenuBar::ShowHelpMenu() {
        if (ImGui::BeginMenu("Help")) {
            // Render views that have "Help" as their top-level category
            if (projectManager.IsProjectOpen()) {
                RenderViewsForCategory("Help");
            }

            ImGui::EndMenu();
        }
    }

    void MenuBar::RenderViewsForCategory(const std::string& categoryName) {
        auto allViews = viewManager.GetRegisteredViews();

        // Build menu tree for this category
        MenuNode rootMenu;
        bool hasViewsInCategory = false;

        for (const auto& [viewTypeName, typeID] : allViews) {
            auto meta = viewManager.GetViewMetadata(viewTypeName);
            auto categoryParts = SplitCategoryPath(meta.category);

            // Skip views with "Hidden" category
            if (!categoryParts.empty() && categoryParts[0] == "Hidden") {
                continue;
            }

            // Skip if this view doesn't belong to the requested category
            if (categoryParts.empty() || categoryParts[0] != categoryName) {
                continue;
            }

            hasViewsInCategory = true;

            // Remove the top-level category part since we're already in that menu
            std::vector<std::string> subCategoryParts(categoryParts.begin() + 1, categoryParts.end());

            // Navigate/create the menu tree for subcategories
            MenuNode* currentNode = &rootMenu;
            for (const auto& part : subCategoryParts) {
                auto it = currentNode->children.find(part);
                if (it == currentNode->children.end()) {
                    currentNode->children[part] = std::make_unique<MenuNode>();
                }
                currentNode = currentNode->children[part].get();
            }

            // Add the view to the final menu level
            currentNode->views.push_back({ viewTypeName, meta.displayName });
        }

        // Only render if we have views in this category
        if (hasViewsInCategory) {
            // Add separator if menu already has items
            if (ImGui::GetCursorPosY() > ImGui::GetFrameHeightWithSpacing()) {
                ImGui::Separator();
            }
            RenderMenuNode(rootMenu);
        }
    }

    std::vector<std::string> MenuBar::GetCustomTopLevelCategories() const {
        std::set<std::string> customCategories;
        auto allViews = viewManager.GetRegisteredViews();

        // Standard menus that shouldn't be created dynamically
        std::set<std::string> standardMenus = { "File", "Edit", "Workspace", "Help" };

        for (const auto& [viewTypeName, typeID] : allViews) {
            auto meta = viewManager.GetViewMetadata(viewTypeName);
            auto categoryParts = SplitCategoryPath(meta.category);

            // Skip hidden views
            if (!categoryParts.empty() && categoryParts[0] == "Hidden") {
                continue;
            }

            // Skip if no category
            if (categoryParts.empty()) {
                continue;
            }

            std::string topLevelCategory = categoryParts[0];

            // Only add if not a standard menu
            if (standardMenus.find(topLevelCategory) == standardMenus.end()) {
                customCategories.insert(topLevelCategory);
            }
        }

        return std::vector<std::string>(customCategories.begin(), customCategories.end());
    }

    void MenuBar::RenderWorkspaceDialogs() {
        if (showCreateWorkspaceDialog) {
            ImGui::OpenPopup("Create Workspace");
        }

        if (ImGui::BeginPopupModal("Create Workspace", &showCreateWorkspaceDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter workspace name:");
            ImGui::InputText("##workspace_name", createWorkspaceBuffer, sizeof(createWorkspaceBuffer));

            std::string proposedName(createWorkspaceBuffer);
            bool nameTaken = viewManager.IsWorkspaceNameTaken(proposedName);
            if (nameTaken && !proposedName.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::Text("Warning: Name already exists!");
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            bool canCreate = !proposedName.empty() && !nameTaken;
            if (!canCreate) ImGui::BeginDisabled();

            if (ImGui::Button("Create")) {
                std::unordered_map<std::string, std::string> eventData;
                eventData["workspaceName"] = std::string(createWorkspaceBuffer);
                ANI::Events::Ref().QueueEventWithData("CreateWorkspace", eventData);

                showCreateWorkspaceDialog = false;
                strcpy(createWorkspaceBuffer, "New Workspace");

                std::cout << "[MenuBar] Queued CreateWorkspace event: " << eventData["workspaceName"] << std::endl;
            }

            if (!canCreate) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showCreateWorkspaceDialog = false;
                strcpy(createWorkspaceBuffer, "New Workspace");
            }

            ImGui::EndPopup();
        }

        if (showRenameWorkspaceDialog) {
            ImGui::OpenPopup("Rename Workspace");
        }

        if (ImGui::BeginPopupModal("Rename Workspace", &showRenameWorkspaceDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter new workspace name:");
            ImGui::InputText("##rename_workspace", renameWorkspaceBuffer, sizeof(renameWorkspaceBuffer));

            std::string proposedName(renameWorkspaceBuffer);
            GUI::WorkspaceID currentWorkspace = projectManager.GetViewState().GetLastActiveWorkspace();
            bool nameTaken = viewManager.IsWorkspaceNameTaken(proposedName, currentWorkspace);
            if (nameTaken && !proposedName.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::Text("Warning: Name already exists!");
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            bool canRename = !proposedName.empty() && !nameTaken;
            if (!canRename) ImGui::BeginDisabled();

            if (ImGui::Button("Rename")) {
                // Direct call - no event needed for simple rename
                viewManager.SetWorkspaceName(currentWorkspace, std::string(renameWorkspaceBuffer));
                showRenameWorkspaceDialog = false;
            }

            if (!canRename) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                showRenameWorkspaceDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    std::vector<std::string> MenuBar::SplitCategoryPath(const std::string& category) const {
        std::vector<std::string> parts;
        std::stringstream ss(category);
        std::string part;

        while (std::getline(ss, part, '/')) {
            if (!part.empty()) {
                size_t start = part.find_first_not_of(" \t");
                size_t end = part.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    parts.push_back(part.substr(start, end - start + 1));
                }
            }
        }

        return parts;
    }

    void MenuBar::RenderMenuNode(const MenuNode& node) {
        // Sort children alphabetically
        std::vector<std::pair<std::string, MenuNode*>> sortedChildren;
        for (const auto& [name, child] : node.children) {
            sortedChildren.push_back({ name, child.get() });
        }
        std::sort(sortedChildren.begin(), sortedChildren.end());

        // Sort views by display name
        std::vector<std::pair<std::string, std::string>> sortedViews = node.views;
        std::sort(sortedViews.begin(), sortedViews.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

        // Render child menus (submenus) first
        for (const auto& [menuName, childNode] : sortedChildren) {
            if (ImGui::BeginMenu(menuName.c_str())) {
                RenderMenuNode(*childNode);
                ImGui::EndMenu();
            }
        }

        // Add separator if we have both submenus and views
        if (!sortedChildren.empty() && !sortedViews.empty()) {
            ImGui::Separator();
        }

        // Render views in this menu level
        for (const auto& [viewTypeName, displayName] : sortedViews) {
            bool isViewActive = IsViewActiveInCurrentWorkspace(viewTypeName);

            if (ImGui::MenuItem(displayName.c_str(), nullptr, isViewActive)) {
                ToggleViewInCurrentWorkspace(viewTypeName);
            }
        }
    }

    void MenuBar::CreateNewWorkspace() {
        std::unordered_map<std::string, std::string> eventData;
        eventData["workspaceName"] = "New Workspace";
        ANI::Events::Ref().QueueEventWithData("CreateWorkspace", eventData);

        std::cout << "[MenuBar] Queued CreateWorkspace event: " << eventData["workspaceName"] << std::endl;
    }

    void MenuBar::DeleteCurrentWorkspace() {
        auto allWorkspaces = viewManager.GetAllWorkspaces();
        if (allWorkspaces.size() <= 1) {
            std::cout << "[MenuBar] Cannot delete the last workspace" << std::endl;
            return;
        }

        GUI::WorkspaceID currentWorkspace = projectManager.GetViewState().GetLastActiveWorkspace();
        ANI::Events::Ref().QueueEventWithData("DeleteWorkspace", currentWorkspace);

        std::cout << "[MenuBar] Queued DeleteWorkspace event for workspace: " << currentWorkspace << std::endl;
    }

    bool MenuBar::IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const {
        try {
            GUI::ViewTypeID viewTypeID = viewManager.GetViewType(viewTypeName);
            const auto& workspaceSignatures = viewManager.GetWorkspaceSignatures();
            GUI::WorkspaceID currentWorkspace = projectManager.GetViewState().GetLastActiveWorkspace();

            auto it = workspaceSignatures.find(currentWorkspace);
            if (it != workspaceSignatures.end()) {
                return it->second->count(viewTypeID) > 0;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MenuBar] Error checking view state: " << e.what() << std::endl;
        }
        return false;
    }

    void MenuBar::ToggleViewInCurrentWorkspace(const std::string& viewTypeName) {
        try {
            GUI::WorkspaceID currentWorkspace = projectManager.GetViewState().GetLastActiveWorkspace();

            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = currentWorkspace;
            eventData["viewTypeName"] = viewTypeName;

            if (IsViewActiveInCurrentWorkspace(viewTypeName)) {
                // Queue RemoveView event
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
                std::cout << "[MenuBar] Queued RemoveView event: " << viewTypeName
                    << " from workspace: " << currentWorkspace << std::endl;
            }
            else {
                // Queue AddView event
                ANI::Events::Ref().QueueEventWithData("AddView", eventData);
                std::cout << "[MenuBar] Queued AddView event: " << viewTypeName
                    << " to workspace: " << currentWorkspace << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[MenuBar] Error toggling view: " << e.what() << std::endl;
        }
    }

} // namespace GUI