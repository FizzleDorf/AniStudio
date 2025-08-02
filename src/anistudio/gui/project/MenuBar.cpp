#include "MenuBar.hpp"
#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>

namespace GUI {

	MenuBar::MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr)
		: m_projectManager(projectMgr), m_viewManager(viewMgr),
		m_newProjectView(projectMgr), m_loadProjectView(projectMgr) {

		// Initialize the views
		m_newProjectView.Init();
		m_loadProjectView.Init();
	}

	void MenuBar::Update(float deltaTime) {
		// Update the views
		m_newProjectView.Update(deltaTime);
		m_loadProjectView.Update(deltaTime);
	}

	void MenuBar::Render() {
		// Always render the popup views first
		m_newProjectView.Render();
		m_loadProjectView.Render();

		// Only render menubar if inside a window with menubar enabled
		if (ImGui::BeginMenuBar()) {
			ShowFileMenu();
			ShowEditMenu();
			ShowViewMenus();
			ShowWorkspaceMenu();
			ShowHelpMenu();
			ImGui::EndMenuBar();
		}
	}

	void MenuBar::ShowFileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Project...", "Ctrl+N")) {
				// Open the new project view
				m_projectManager.GetViewState().SetViewOpen("NewProjectView", true);
			}

			if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
				// Open the load project view
				m_projectManager.GetViewState().SetViewOpen("LoadProjectView", true);
			}

			ImGui::Separator();

			bool projectOpen = m_projectManager.IsProjectOpen();
			if (ImGui::MenuItem("Save Project", "Ctrl+S", false, projectOpen)) {
				m_projectManager.SaveProject();
			}

			if (ImGui::MenuItem("Close Project", "", false, projectOpen)) {
				m_projectManager.CloseProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				exit(0);
			}

			ImGui::EndMenu();
		}
	}

	void MenuBar::ShowEditMenu() {
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
				// TODO: Implement undo
			}

			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
				// TODO: Implement redo
			}

			ImGui::Separator();
			ImGui::EndMenu();
		}
	}

	void MenuBar::ShowViewMenus() {
		if (!m_projectManager.IsProjectOpen()) return;

		if (ImGui::BeginMenu("View")) {
			// Get all registered views from ViewManager
			auto allViews = m_viewManager.GetRegisteredViews();

			// Build menu tree structure
			MenuNode rootMenu;

			for (const auto&[viewTypeName, typeID] : allViews) {
				auto meta = m_viewManager.GetViewMetadata(viewTypeName);
				auto categoryParts = SplitCategoryPath(meta.category);

				// Skip views with "Hidden" category
				if (!categoryParts.empty() && categoryParts[0] == "Hidden") {
					continue;
				}

				// If no category parts, put in "Other"
				if (categoryParts.empty()) {
					categoryParts.push_back("Other");
				}

				// Navigate/create the menu tree
				MenuNode* currentNode = &rootMenu;
				for (const auto& part : categoryParts) {
					auto it = currentNode->children.find(part);
					if (it == currentNode->children.end()) {
						currentNode->children[part] = std::make_unique<MenuNode>();
					}
					currentNode = currentNode->children[part].get();
				}

				// Add the view to the final menu level
				currentNode->views.push_back({ viewTypeName, meta.displayName });
			}

			// Render the menu tree starting from root
			RenderMenuNode(rootMenu);

			ImGui::EndMenu();
		}
	}

	void MenuBar::ShowWorkspaceMenu() {
		if (!m_projectManager.IsProjectOpen()) return;

		if (ImGui::BeginMenu("Workspace")) {
			auto& viewState = m_projectManager.GetViewState();
			auto workspaceList = viewState.GetWorkspaceList();
			size_t activeWorkspaceID = viewState.GetActiveWorkspaceID();

			// Show current active workspace
			std::string activeWorkspaceName = "None";
			for (const auto&[workspaceID, alias] : workspaceList) {
				if (workspaceID == activeWorkspaceID) {
					activeWorkspaceName = alias;
					break;
				}
			}

			ImGui::Text("Active: %s", activeWorkspaceName.c_str());
			ImGui::Separator();

			// List all workspaces as radio buttons
			for (const auto&[workspaceID, alias] : workspaceList) {
				bool isActive = (workspaceID == activeWorkspaceID);
				if (ImGui::MenuItem(alias.c_str(), nullptr, isActive)) {
					if (!isActive) {
						viewState.SetActiveWorkspace(workspaceID);
						std::cout << "[MenuBar] Switched to workspace: " << alias << " (ID: " << workspaceID << ")" << std::endl;
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("New Workspace")) {
				size_t newID = viewState.CreateWorkspace("New Workspace", {});
				std::cout << "[MenuBar] Created new workspace with ID: " << newID << std::endl;
			}

			// Delete current workspace (only if more than one exists)
			if (workspaceList.size() > 1) {
				if (ImGui::MenuItem("Delete Current Workspace")) {
					std::string workspaceToDelete = activeWorkspaceName;
					if (viewState.DeleteWorkspace(activeWorkspaceID)) {
						std::cout << "[MenuBar] Deleted workspace: " << workspaceToDelete << " (ID: " << activeWorkspaceID << ")" << std::endl;
					}
				}
			}

			// Rename current workspace
			if (ImGui::MenuItem("Rename Current Workspace")) {
				ImGui::OpenPopup("RenameWorkspace");
			}

			// Rename workspace popup
			if (ImGui::BeginPopupModal("RenameWorkspace", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				static char renameBuffer[256] = "";
				static bool justOpened = true;

				if (justOpened) {
					strcpy_s(renameBuffer, activeWorkspaceName.c_str());
					justOpened = false;
				}

				ImGui::Text("Rename workspace: %s", activeWorkspaceName.c_str());
				ImGui::InputText("New Name", renameBuffer, sizeof(renameBuffer));

				if (ImGui::Button("OK")) {
					std::string newAlias = std::string(renameBuffer);
					if (!newAlias.empty() && newAlias != activeWorkspaceName) {
						viewState.RenameWorkspace(activeWorkspaceID, newAlias);
						std::cout << "[MenuBar] Renamed workspace from '" << activeWorkspaceName << "' to '" << newAlias << "'" << std::endl;
					}
					justOpened = true;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					justOpened = true;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::EndMenu();
		}
	}

	void MenuBar::ShowHelpMenu() {
		if (ImGui::BeginMenu("Help")) {
			if (ImGui::MenuItem("About")) {
				// TODO: Show about dialog
			}
			ImGui::EndMenu();
		}
	}

	std::vector<std::string> MenuBar::SplitCategoryPath(const std::string& category) {
		std::vector<std::string> parts;
		std::stringstream ss(category);
		std::string part;

		while (std::getline(ss, part, '/')) {
			if (!part.empty()) {
				// Trim whitespace
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
		// Sort children by name for consistent ordering
		std::vector<std::pair<std::string, MenuNode*>> sortedChildren;
		for (const auto&[name, child] : node.children) {
			sortedChildren.push_back({ name, child.get() });
		}
		std::sort(sortedChildren.begin(), sortedChildren.end());

		// Sort views by display name
		std::vector<std::pair<std::string, std::string>> sortedViews = node.views;
		std::sort(sortedViews.begin(), sortedViews.end(),
			[](const auto& a, const auto& b) {
			return a.second < b.second; // Sort by display name
		});

		// Render child menus (submenus) first
		for (const auto&[menuName, childNode] : sortedChildren) {
			if (ImGui::BeginMenu(menuName.c_str())) {
				RenderMenuNode(*childNode);
				ImGui::EndMenu();
			}
		}

		// Add separator if we have both submenus and views
		if (!sortedChildren.empty() && !sortedViews.empty()) {
			ImGui::Separator();
		}

		// Render views in this menu level - USE VIEWSTATE SYSTEM
		for (const auto&[viewTypeName, displayName] : sortedViews) {
			// Check if this view is open in the active workspace using ViewState
			bool isViewOpen = m_projectManager.GetViewState().IsViewOpen(viewTypeName);

			if (ImGui::MenuItem(displayName.c_str(), nullptr, isViewOpen)) {
				// Toggle the view in the active workspace using ViewState
				m_projectManager.GetViewState().ToggleView(viewTypeName);
				std::cout << "[MenuBar] Toggled view: " << viewTypeName
					<< " (now " << (isViewOpen ? "closed" : "open") << ")" << std::endl;
			}
		}
	}

} // namespace GUI