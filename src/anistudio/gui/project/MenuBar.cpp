#include "MenuBar.hpp"
#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "Events.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>

namespace GUI {

	MenuBar::MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr)
		: projectManager(projectMgr), viewManager(viewMgr), events(ANI::Events::Ref()),
		newProjectView(projectMgr), loadProjectView(projectMgr) {

		// Initialize the views
		newProjectView.Init();
		loadProjectView.Init();
	}

	void MenuBar::Update(float deltaTime) {
		// Update the views
		newProjectView.Update(deltaTime);
		loadProjectView.Update(deltaTime);
	}

	void MenuBar::Render() {
		// Always render the popup views first
		if (showNewProjectDialog) {
			newProjectView.SetVisible(true);
			showNewProjectDialog = false;
		}

		if (showLoadProjectDialog) {
			loadProjectView.SetVisible(true);
			showLoadProjectDialog = false;
		}

		newProjectView.Render();
		loadProjectView.Render();

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
				showNewProjectDialog = true;
			}

			if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
				showLoadProjectDialog = true;
			}

			ImGui::Separator();

			bool projectOpen = projectManager.IsProjectOpen();
			if (ImGui::MenuItem("Save Project", "Ctrl+S", false, projectOpen)) {
				projectManager.SaveProject();
			}

			if (ImGui::MenuItem("Close Project", "", false, projectOpen)) {
				projectManager.CloseProject();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4")) {
				ANI::Event exitEvent;
				exitEvent.type = ANI::EventType::Quit;
				events.QueueEvent(exitEvent);
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
		if (!projectManager.IsProjectOpen()) return;

		if (ImGui::BeginMenu("View")) {
			// Get all registered views from ViewManager
			auto allViews = viewManager.GetRegisteredViews();

			// Build menu tree structure
			MenuNode rootMenu;

			for (const auto&[viewTypeName, typeID] : allViews) {
				auto meta = viewManager.GetViewMetadata(viewTypeName);
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
		if (!projectManager.IsProjectOpen()) return;

		if (ImGui::BeginMenu("Workspace")) {
			auto allWorkspaces = viewManager.GetAllWorkspaces();

			// Show current active workspace
			ImGui::Text("Active: Workspace %zu", projectManager.GetViewState().GetLastActiveWorkspace());
			ImGui::Separator();

			// List all workspaces as radio buttons
			for (GUI::WorkspaceID workspaceID : allWorkspaces) {
				bool isActive = (workspaceID == projectManager.GetViewState().GetLastActiveWorkspace());
				std::string label = "Workspace " + std::to_string(workspaceID);

				if (ImGui::MenuItem(label.c_str(), nullptr, isActive)) {
					if (!isActive) {
						ANI::ViewEvent event;
						event.type = ANI::ViewEventType::SetActiveWorkspace;
						event.workspaceID = workspaceID;
						events.QueueViewEvent(event);
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("New Workspace")) {
				CreateNewWorkspace();
			}

			// Delete current workspace (only if more than one exists)
			if (allWorkspaces.size() > 1) {
				if (ImGui::MenuItem("Delete Current Workspace")) {
					DeleteCurrentWorkspace();
				}
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

		// Render views in this menu level
		for (const auto&[viewTypeName, displayName] : sortedViews) {
			// Check if this view is active in the current workspace
			bool isViewActive = IsViewActiveInCurrentWorkspace(viewTypeName);

			if (ImGui::MenuItem(displayName.c_str(), nullptr, isViewActive)) {
				// Toggle the view in the current workspace
				ToggleViewInCurrentWorkspace(viewTypeName);
			}
		}
	}

	void MenuBar::CreateNewWorkspace() {
		ANI::ViewEvent event;
		event.type = ANI::ViewEventType::CreateWorkspace;
		events.QueueViewEvent(event);
	}

	void MenuBar::DeleteCurrentWorkspace() {
		auto allWorkspaces = viewManager.GetAllWorkspaces();
		if (allWorkspaces.size() <= 1) {
			std::cout << "[MenuBar] Cannot delete the last workspace" << std::endl;
			return;
		}

		ANI::ViewEvent event;
		event.type = ANI::ViewEventType::DeleteWorkspace;
		event.workspaceID = projectManager.GetViewState().GetLastActiveWorkspace();
		events.QueueViewEvent(event);
	}

	bool MenuBar::IsViewActiveInCurrentWorkspace(const std::string& viewTypeName) const {
		try {
			GUI::ViewTypeID viewTypeID = viewManager.GetViewType(viewTypeName);
			const auto& workspaceSignatures = viewManager.GetWorkspaceSignatures();

			auto it = workspaceSignatures.find(projectManager.GetViewState().GetLastActiveWorkspace());
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
			GUI::ViewTypeID viewTypeID = viewManager.GetViewType(viewTypeName);

			if (IsViewActiveInCurrentWorkspace(viewTypeName)) {
				// Remove the view using events
				ANI::ViewEvent event;
				event.type = ANI::ViewEventType::RemoveView;
				event.workspaceID = projectManager.GetViewState().GetLastActiveWorkspace();
				event.viewTypeName = viewTypeName;
				events.QueueViewEvent(event);
			}
			else {
				// Add the view using events
				ANI::ViewEvent event;
				event.type = ANI::ViewEventType::AddView;
				event.workspaceID = projectManager.GetViewState().GetLastActiveWorkspace();
				event.viewTypeName = viewTypeName;
				events.QueueViewEvent(event);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Error toggling view: " << e.what() << std::endl;
		}
	}

} // namespace GUI