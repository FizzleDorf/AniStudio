#include "MenuBar.hpp"
#include "ProjectSystem.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include "SettingsView.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace GUI {

    MenuBar::MenuBar(ANI::ProjectSystem& projectSys, ViewManager& viewMgr, ANI::StudioCore& m_studioCore)
        : projectSystem(projectSys), viewManager(viewMgr), m_studioCore(m_studioCore) {

        popupState.InitializeBuffers(projectSys);
        popupState.LoadTemplates(projectSys);
        popupState.RefreshRecentProjects(projectSys);

        std::cout << "[MenuBar] Constructor - Settings will be accessed via StudioCore" << std::endl;
    }

    void MenuBar::Update(float deltaTime) {
        // Nothing to update
    }

    void MenuBar::Render() {
        ProjectPopups::RenderNewProjectPopup(popupState, projectSystem);
        ProjectPopups::RenderLoadProjectPopup(popupState, projectSystem);

        if (ImGui::BeginMenuBar()) {
            ShowFileMenu();
            // ShowEditMenu();
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
                popupState.RefreshRecentProjects(projectSystem);
            }

            ImGui::Separator();

            bool projectOpen = projectSystem.IsProjectOpen();
            if (ImGui::MenuItem("Save Project", "Ctrl+S", false, projectOpen)) {
                ANI::Events::Ref().QueueEvent("SaveProject");
            }

            if (ImGui::MenuItem("Close Project", "", false, projectOpen)) {
                ANI::Events::Ref().QueueEvent("CloseProject");
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Settings...", "Ctrl+,")) {
                m_studioCore.GetSettingsView().Show();
                std::cout << "[MenuBar] Opening Settings dialog" << std::endl;
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
            if (projectSystem.IsProjectOpen()) {
                RenderViewsForCategory("Edit");
            }

            ImGui::Separator();
            ImGui::EndMenu();
        }
    }

    void MenuBar::ShowWorkspaceMenu() {
        if (!projectSystem.IsProjectOpen()) return;

        if (ImGui::BeginMenu("Workspace")) {
            auto allWorkspaces = viewManager.GetAllWorkspaces();
            GUI::WorkspaceID currentActiveWorkspace = projectSystem.GetViewState().GetLastActiveWorkspace();

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
        if (!projectSystem.IsProjectOpen()) return;

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
            if (projectSystem.IsProjectOpen()) {
                RenderViewsForCategory("Help");
            }

            ImGui::EndMenu();
        }
    }

    void MenuBar::RenderViewsForCategory(const std::string& categoryName) {
        auto allViews = viewManager.GetRegisteredViews();

        MenuNode rootMenu;
        bool hasViewsInCategory = false;

        for (const auto& [viewTypeName, typeID] : allViews) {
            auto meta = viewManager.GetViewMetadata(viewTypeName);
            auto categoryParts = SplitCategoryPath(meta.category);

            if (!categoryParts.empty() && categoryParts[0] == "Hidden") {
                continue;
            }

            if (categoryParts.empty() || categoryParts[0] != categoryName) {
                continue;
            }

            hasViewsInCategory = true;

            std::vector<std::string> subCategoryParts(categoryParts.begin() + 1, categoryParts.end());

            MenuNode* currentNode = &rootMenu;
            for (const auto& part : subCategoryParts) {
                auto it = currentNode->children.find(part);
                if (it == currentNode->children.end()) {
                    currentNode->children[part] = std::make_unique<MenuNode>();
                }
                currentNode = currentNode->children[part].get();
            }

            currentNode->views.push_back({ viewTypeName, meta.displayName });
        }

        if (hasViewsInCategory) {
            if (ImGui::GetCursorPosY() > ImGui::GetFrameHeightWithSpacing()) {
                ImGui::Separator();
            }
            RenderMenuNode(rootMenu);
        }
    }

    std::vector<std::string> MenuBar::GetCustomTopLevelCategories() const {
        std::set<std::string> customCategories;
        auto allViews = viewManager.GetRegisteredViews();

        std::set<std::string> standardMenus = { "File", /*"Edit",*/ "Workspace", "Help" };

        for (const auto& [viewTypeName, typeID] : allViews) {
            auto meta = viewManager.GetViewMetadata(viewTypeName);
            auto categoryParts = SplitCategoryPath(meta.category);

            if (!categoryParts.empty() && categoryParts[0] == "Hidden") {
                continue;
            }

            if (categoryParts.empty()) {
                continue;
            }

            std::string topLevelCategory = categoryParts[0];

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
            GUI::WorkspaceID currentWorkspace = projectSystem.GetViewState().GetLastActiveWorkspace();
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
        std::vector<std::pair<std::string, MenuNode*>> sortedChildren;
        for (const auto& [name, child] : node.children) {
            sortedChildren.push_back({ name, child.get() });
        }
        std::sort(sortedChildren.begin(), sortedChildren.end());

        std::vector<std::pair<std::string, std::string>> sortedViews = node.views;
        std::sort(sortedViews.begin(), sortedViews.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

        for (const auto& [menuName, childNode] : sortedChildren) {
            if (ImGui::BeginMenu(menuName.c_str())) {
                RenderMenuNode(*childNode);
                ImGui::EndMenu();
            }
        }

        if (!sortedChildren.empty() && !sortedViews.empty()) {
            ImGui::Separator();
        }

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

        GUI::WorkspaceID currentWorkspace = projectSystem.GetViewState().GetLastActiveWorkspace();
        ANI::Events::Ref().QueueEventWithData("DeleteWorkspace", currentWorkspace);

        std::cout << "[MenuBar] Queued DeleteWorkspace event for workspace: " << currentWorkspace << std::endl;
    }

    bool MenuBar::IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const {
        try {
            GUI::ViewTypeID viewTypeID = viewManager.GetViewType(viewTypeName);
            const auto& workspaceSignatures = viewManager.GetWorkspaceSignatures();
            GUI::WorkspaceID currentWorkspace = projectSystem.GetViewState().GetLastActiveWorkspace();

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
            GUI::WorkspaceID currentWorkspace = projectSystem.GetViewState().GetLastActiveWorkspace();

            std::unordered_map<std::string, std::any> eventData;
            eventData["workspaceID"] = currentWorkspace;
            eventData["viewTypeName"] = viewTypeName;

            if (IsViewActiveInCurrentWorkspace(viewTypeName)) {
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
                std::cout << "[MenuBar] Queued RemoveView event: " << viewTypeName
                    << " from workspace: " << currentWorkspace << std::endl;
            }
            else {
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