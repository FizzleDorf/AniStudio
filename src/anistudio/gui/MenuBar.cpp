// Updated MenuBar.cpp with top-level category menus
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

	void MenuBar::Render() {
		if (ImGui::BeginMainMenuBar()) {
			ShowFileMenu();
			ShowEditMenu();
			ShowHelpMenu();
			// Dynamic category-based menus
			ShowCategoryMenus();
			ImGui::EndMainMenuBar();
		}

		UpdateViews();
	}

	void MenuBar::UpdateViews() {
		SyncViewState();
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

			ImGui::Separator();

			ImGui::EndMenu();
		}
	}

	void MenuBar::ShowEditMenu() {
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
				// TODO: ImGui undo
			}

			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
				// TODO: ImGui redo
			}

			ImGui::Separator();
			ImGui::EndMenu();
		}
	}

	// this just ensures the help menu is always placed last
	void MenuBar::ShowHelpMenu() {
		if (ImGui::BeginMenu("Help")) {
			ImGui::EndMenu();
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

	// Helper function to split category path
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

	// Render a menu node and its children (for submenus)
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
			bool isOpen = m_projectManager.GetViewState().IsViewOpen(viewTypeName);

			if (ImGui::MenuItem(meta.displayName.c_str(), nullptr, isOpen)) {
				m_projectManager.GetViewState().ToggleView(viewTypeName);
			}

			if (ImGui::IsItemHovered() && !meta.description.empty()) {
				ImGui::SetTooltip("%s", meta.description.c_str());
			}
		}
	}

	void MenuBar::SyncViewState() {
		auto& viewState = m_projectManager.GetViewState();
		auto openViewTypes = viewState.GetOpenViewTypes();

		// Create views that should be open but aren't active
		for (const auto& viewType : openViewTypes) {
			if (!IsViewActive(viewType)) {
				CreateView(viewType);
			}
		}

		// Destroy views that are active but shouldn't be
		auto it = m_activeViews.begin();
		while (it != m_activeViews.end()) {
			const std::string& viewType = it->first;
			if (!viewState.IsViewOpen(viewType)) {
				ViewListID viewID = it->second;
				it = m_activeViews.erase(it);
				m_viewManager.DestroyView(viewID);
			}
			else {
				++it;
			}
		}
	}

	void MenuBar::CreateView(const std::string& viewType) {
		if (IsViewActive(viewType)) return;

		try {
			ViewListID viewID = m_viewManager.CreateViewByName(viewType, m_entityManager);

			if (viewID != 0) {
				m_activeViews[viewType] = viewID;
				std::cout << "[MenuBar] Created view: " << viewType << " with ID: " << viewID << std::endl;
			}
			else {
				std::cerr << "[MenuBar] Failed to create view: " << viewType << std::endl;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[MenuBar] Exception creating view " << viewType << ": " << e.what() << std::endl;
		}
	}

	void MenuBar::DestroyView(const std::string& viewType) {
		auto it = m_activeViews.find(viewType);
		if (it != m_activeViews.end()) {
			m_viewManager.DestroyView(it->second);
			m_activeViews.erase(it);
		}
	}

	bool MenuBar::IsViewActive(const std::string& viewType) const {
		return m_activeViews.find(viewType) != m_activeViews.end();
	}

} // namespace GUI