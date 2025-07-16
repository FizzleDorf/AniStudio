// Updated MenuBar.cpp - Properly managing actual view instances
#include "MenuBar.hpp"
#include "../Events/Events.hpp"
#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "AllViews.h"
#include <imgui.h>
#include <algorithm>
#include <sstream>

namespace GUI {

	MenuBar::MenuBar(ANI::ProjectManager& projectMgr, ViewManager& viewMgr, ECS::EntityManager& entityMgr)
		: BaseView(entityMgr), m_projectManager(projectMgr), m_viewManager(viewMgr), m_entityManager(entityMgr) {
		viewName = "MenuBar";
	}

	void MenuBar::Init() {
		// Create a workspace for this menubar to manage if one isn't set
		if (m_managedWorkspace == 0) {
			m_managedWorkspace = m_viewManager.CreateView();
			std::cout << "[MenuBar] Created managed workspace: " << m_managedWorkspace << std::endl;
		}
	}

	void MenuBar::Update(const float deltaT) {
		SyncViewState();
	}

	void MenuBar::Render() {
		if (ImGui::BeginMainMenuBar()) {
			ShowFileMenu();
			ShowEditMenu();
			ShowHelpMenu();
			// Dynamic category-based menus
			ShowCategoryMenus();
			ImGui::EndMainMenuBar();
		}
	}

	void MenuBar::ShowFileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Project...", "Ctrl+N")) {
				m_projectManager.GetViewState().SetViewOpen("NewProjectView", true);
			}

			if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
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
				ANI::Event event;
				event.type = ANI::EventType::Quit;
				ANI::Events::Ref().QueueEvent(event);
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

	void MenuBar::ShowHelpMenu() {
		if (ImGui::BeginMenu("Help")) {
			
		}
	}

	void MenuBar::ShowCategoryMenus() {
		// Get all registered views
		auto allViews = m_viewManager.GetRegisteredViews();

		// Build menu tree structure
		MenuNode rootMenu;

		for (const auto&[viewTypeName, typeID] : allViews) {
			auto meta = m_viewManager.GetViewMetadata(viewTypeName);
			auto categoryParts = SplitCategoryPath(meta.category);

			// Skip views with "Hidden/hidden" category
			if (!categoryParts.empty() &&
				(categoryParts[0] == "Hidden" || categoryParts[0] == "hidden")) {
				continue;  // Skip this view entirely
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
			currentNode->views.push_back({ viewTypeName, meta });
		}

		// Render each top-level category as its own main menu
		std::vector<std::pair<std::string, MenuNode*>> sortedTopLevel;
		for (const auto&[name, child] : rootMenu.children) {
			sortedTopLevel.push_back({ name, child.get() });
		}
		std::sort(sortedTopLevel.begin(), sortedTopLevel.end());

		for (const auto&[categoryName, categoryNode] : sortedTopLevel) {
			if (ImGui::BeginMenu(categoryName.c_str())) {
				RenderMenuNode(*categoryNode);
				ImGui::EndMenu();
			}
		}
	}

	std::vector<std::string> MenuBar::SplitCategoryPath(const std::string& category) {
		std::vector<std::string> parts;
		std::stringstream ss(category);
		std::string part;

		while (std::getline(ss, part, '/')) {
			if (!part.empty()) {
				parts.push_back(part);
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

		// Render child menus (submenus)
		for (const auto&[menuName, childNode] : sortedChildren) {
			if (ImGui::BeginMenu(menuName.c_str())) {
				RenderMenuNode(*childNode);
				ImGui::EndMenu();
			}
		}

		// Add separator if we have both submenus and views
		if (!sortedChildren.empty() && !node.views.empty()) {
			ImGui::Separator();
		}

		// Render views in this menu level
		for (const auto&[viewTypeName, meta] : node.views) {
			bool isViewActive = IsViewInstanceInWorkspace(viewTypeName);

			if (ImGui::MenuItem(meta.displayName.c_str(), nullptr, isViewActive)) {
				ToggleViewInstanceInWorkspace(viewTypeName);
			}

			if (ImGui::IsItemHovered() && !meta.description.empty()) {
				ImGui::SetTooltip("%s", meta.description.c_str());
			}
		}
	}

	void MenuBar::SyncViewState() {
		// Sync the workspace state with the project's view state
		auto& viewState = m_projectManager.GetViewState();
		auto openViewTypes = viewState.GetOpenViewTypes();

		// For each view type that should be open, ensure it exists in workspace
		for (const auto& viewType : openViewTypes) {
			if (!IsViewInstanceInWorkspace(viewType)) {
				CreateViewInstance(viewType);
			}
		}

		// For each view type that shouldn't be open, remove from workspace
		for (const auto&[viewTypeName, typeId] : m_viewManager.GetRegisteredViews()) {
			bool shouldBeOpen = std::find(openViewTypes.begin(), openViewTypes.end(), viewTypeName) != openViewTypes.end();
			if (!shouldBeOpen && IsViewInstanceInWorkspace(viewTypeName)) {
				RemoveViewInstance(viewTypeName);
			}
		}
	}

	bool MenuBar::IsViewInstanceInWorkspace(const std::string& viewTypeName) const {
		// We need to check if the actual view instance exists in the workspace
		// This requires us to know the concrete type to call HasView<T>()
		// For now, we'll use a registry of known view types
		return CheckViewExistsByName(viewTypeName);
	}

	void MenuBar::ToggleViewInstanceInWorkspace(const std::string& viewType) {
		if (IsViewInstanceInWorkspace(viewType)) {
			RemoveViewInstance(viewType);
			m_projectManager.GetViewState().SetViewOpen(viewType, false);
		}
		else {
			CreateViewInstance(viewType);
			m_projectManager.GetViewState().SetViewOpen(viewType, true);
		}
	}

	void MenuBar::CreateViewInstance(const std::string& viewTypeName) {
		// Create the actual view instance using the registered factory
		try {
			// Use CreateViewByName to create in generic storage, then add to our workspace
			WorkspaceID tempId = m_viewManager.CreateViewByName(viewTypeName, m_entityManager);
			if (tempId != 0) {
				// Move the view from generic storage to our managed workspace
				// For now, just track it in our registry
				m_activeViewInstances[viewTypeName] = tempId;
				std::cout << "[MenuBar] Created view instance: " << viewTypeName << " with ID: " << tempId << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Failed to create view instance " << viewTypeName << ": " << e.what() << std::endl;
		}
	}

	void MenuBar::RemoveViewInstance(const std::string& viewTypeName) {
		auto it = m_activeViewInstances.find(viewTypeName);
		if (it != m_activeViewInstances.end()) {
			m_viewManager.DestroyView(it->second);
			m_activeViewInstances.erase(it);
			std::cout << "[MenuBar] Removed view instance: " << viewTypeName << std::endl;
		}
	}

	bool MenuBar::CheckViewExistsByName(const std::string& viewTypeName) const {
		return m_activeViewInstances.find(viewTypeName) != m_activeViewInstances.end();
	}

} // namespace GUI