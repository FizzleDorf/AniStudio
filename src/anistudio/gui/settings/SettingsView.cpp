// SettingsView.cpp
#include "SettingsView.hpp"
#include "PathsSettings.hpp"
#include "GeneralSettings.hpp"
#include "SDCPPSettings.hpp"
#include "ImGuiStyleSettings.hpp"
#include "ImGuiRenderSettings.hpp"
#include "Events.hpp"
#include <iostream>
#include <algorithm>
#include <imgui_internal.h>

namespace GUI {

	SettingsView::SettingsView(ECS::EntityManager &mgr)
		: BaseView(mgr), showSavePopup(false),
		showUnsavedChangesDialog(false), pendingClose(false), windowOpen(true),
		filterListWidth(250.0f) {
		viewName = "SettingsView";

		// Register core tabs
		RegisterCoreTabs();

		// Load all settings on startup
		settingsManager.LoadAllSettings();

		// Initialize with first tab's categories if available
		if (!settingsManager.GetTabs().empty()) {
			UpdateCategoriesForActiveTab(settingsManager.GetTabs()[0]->GetTabName());
		}

		std::cout << "[SettingsView] Initialized" << std::endl;
	}

	void SettingsView::RegisterCoreTabs() {
		std::cout << "[SettingsView] Registering core tabs..." << std::endl;

		settingsManager.RegisterTab(std::make_unique<Settings::GeneralSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::PathsSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::SDCPPSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::ImGuiStyleSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::ImGuiRenderSettingsTab>());

		std::cout << "[SettingsView] Registered core tabs" << std::endl;
	}

	void SettingsView::UpdateCategoriesForActiveTab(const std::string& activeTabName) {
		availableCategories.clear();

		// Find the active tab and get its categories dynamically
		for (const auto& tab : settingsManager.GetTabs()) {
			if (tab->GetTabName() == activeTabName) {
				// Get categories from the tab itself - each tab should provide its own categories
				auto tabCategories = GetCategoriesFromTab(tab.get());
				for (const auto& category : tabCategories) {
					availableCategories.insert(category);
				}
				break;
			}
		}

		// Select all categories by default when switching tabs
		SelectAllCategories();
	}

	std::vector<std::string> SettingsView::GetCategoriesFromTab(Settings::BaseTabObject* tab) {
		// First try to get categories from the tab itself (for plugins)
		auto tabCategories = tab->GetCategories();
		if (!tabCategories.empty()) {
			return tabCategories;
		}

		// Fall back to hardcoded mapping for core tabs
		std::string tabName = tab->GetTabName();

		if (tabName == "General") {
			return { "Startup Settings", "Auto-Save Settings", "Confirmation Dialogs", "Performance Settings", "Logging Settings" };
		}
		else if (tabName == "Paths") {
			return { "General Paths", "Model Paths" };
		}
		else if (tabName == "SDCPP") {
			return { "SDCPP Configuration", "SDCPP Controls" };
		}
		else if (tabName == "ImGui Style") {
			return { "Style Presets", "Font Settings", "Size Settings", "Border Settings", "Rounding Settings", "Color Settings" };
		}
		else if (tabName == "ImGui Render") {
			return { "Display Settings", "Input Settings", "Window Behavior", "Navigation Settings", "Docking Settings", "Multi-Viewport Settings", "Memory & Performance", "Input Text Settings" };
		}

		// For unknown tabs without GetCategories() override, return empty
		return {};
	}

	void SettingsView::RenderFilteredTabContent(Settings::BaseTabObject* tab, const std::set<std::string>& selectedCategories) {
		// Try to use filtered rendering if the tab supports it
		tab->RenderFilteredUI(selectedCategories);
	}

	std::string SettingsView::GetWindowTitle() const {
		return viewName + "###SettingsWindow" + std::to_string(GetID());
	}

	void SettingsView::Render() {
		ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {
			if (!windowOpen) {
				std::unordered_map<std::string, std::any> eventData;
				eventData["workspaceID"] = GetID();
				eventData["viewTypeName"] = viewName;
				ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
				ImGui::End();
			}

			RenderMainContent();

		}
		ImGui::End();

		// Render popups outside of the main window - these should always render
		RenderSaveConfirmationPopup();
		RenderUnsavedChangesDialog();
	}

	void SettingsView::RenderMainContent() {
		// Split the window into filter list and content areas
		ImGui::BeginChild("SettingsSplit", ImVec2(0, -50), false);

		// Left side: Filter list
		ImGui::BeginChild("FilterList", ImVec2(filterListWidth, 0), true);
		RenderFilterList();
		ImGui::EndChild();

		ImGui::SameLine();

		// Right side: Selected categories content
		ImGui::BeginChild("ContentArea", ImVec2(0, 0), true);
		RenderSelectedCategoriesContent();
		ImGui::EndChild();

		ImGui::EndChild();

		// Action buttons at the bottom
		ImGui::Separator();
		RenderActionButtons();
	}

	void SettingsView::RenderFilterList() {
		ImGui::Text("Settings Categories");
		ImGui::Separator();

		// Show All button
		if (ImGui::Button("Show All Categories", ImVec2(-1, 0))) {
			SelectAllCategories();
		}

		if (ImGui::Button("Deselect All", ImVec2(-1, 0))) {
			DeselectAll();
		}

		ImGui::Separator();

		// Render categories list for current tab only
		for (const auto& category : availableCategories) {
			bool isSelected = selectedCategories.find(category) != selectedCategories.end();

			ImGui::PushStyleColor(ImGuiCol_Text, isSelected ?
				ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

			bool clicked = ImGui::Selectable(
				category.c_str(),
				isSelected,
				ImGuiSelectableFlags_AllowDoubleClick
			);

			ImGui::PopStyleColor();

			if (clicked) {
				HandleCategorySelection(category,
					ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift);
			}
		}

		ImGui::Separator();
		ImGui::Text("Selected: %zu categories", selectedCategories.size());
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"Ctrl+Click: Multi-select");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"Shift+Click: Range select");
	}

	void SettingsView::RenderSelectedCategoriesContent() {
		static std::string currentActiveTab = "";

		// Always render tabs
		if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
			for (const auto& tab : settingsManager.GetTabs()) {
				std::string tabTitle = tab->GetTabName() + " Settings";
				std::string tabName = tab->GetTabName();

				if (ImGui::BeginTabItem(tabTitle.c_str())) {
					// Check if we switched to a different tab
					if (currentActiveTab != tabName) {
						currentActiveTab = tabName;
						UpdateCategoriesForActiveTab(tabName);
					}

					ImGui::BeginChild("TabContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

					// Pass the selected categories to the tab for filtering
					RenderFilteredTabContent(tab.get(), selectedCategories);

					ImGui::EndChild();
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		// Initialize with first tab if no tab is active yet
		if (currentActiveTab.empty() && !settingsManager.GetTabs().empty()) {
			currentActiveTab = settingsManager.GetTabs()[0]->GetTabName();
			UpdateCategoriesForActiveTab(currentActiveTab);
		}
	}

	void SettingsView::HandleCategorySelection(const std::string& categoryName, bool ctrlHeld, bool shiftHeld) {
		if (!ctrlHeld && !shiftHeld) {
			// Single selection - clear others and select this category
			selectedCategories.clear();
			selectedCategories.insert(categoryName);
			lastSelectedCategory = categoryName;
		}
		else if (ctrlHeld) {
			// Toggle selection
			if (selectedCategories.count(categoryName)) {
				selectedCategories.erase(categoryName);
			}
			else {
				selectedCategories.insert(categoryName);
			}
			lastSelectedCategory = categoryName;
		}
		else if (shiftHeld && !lastSelectedCategory.empty()) {
			// Range selection
			std::vector<std::string> categoryOrder(availableCategories.begin(), availableCategories.end());
			std::sort(categoryOrder.begin(), categoryOrder.end());

			auto startIt = std::find(categoryOrder.begin(), categoryOrder.end(), lastSelectedCategory);
			auto endIt = std::find(categoryOrder.begin(), categoryOrder.end(), categoryName);

			if (startIt != categoryOrder.end() && endIt != categoryOrder.end()) {
				if (startIt > endIt) std::swap(startIt, endIt);

				for (auto it = startIt; it <= endIt; ++it) {
					selectedCategories.insert(*it);
				}
			}
		}
	}

	void SettingsView::SelectAllCategories() {
		selectedCategories = availableCategories;
	}

	void SettingsView::DeselectAll() {
		selectedCategories.clear();
	}

	void SettingsView::HandleWindowClose() {
		if (settingsManager.HasAnyUnsavedChanges()) {
			showUnsavedChangesDialog = true;
			pendingClose = true;
			windowOpen = true; // Keep window open until user decides
		}
		else {
			// No unsaved changes, close immediately
			std::unordered_map<std::string, std::any> eventData;
			eventData["workspaceID"] = GetID();
			eventData["viewTypeName"] = viewName;
			ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
		}
	}

	void SettingsView::RenderActionButtons() {
		// Show unsaved changes indicator
		if (settingsManager.HasAnyUnsavedChanges()) {
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "* Unsaved changes");
			ImGui::SameLine();
		}

		// Apply button - saves settings but keeps window open
		if (ImGui::Button("Apply")) {
			if (settingsManager.SaveAllSettings()) {
				showSavePopup = true;
			}
		}

		ImGui::SameLine();

		// Save and Close button - saves settings and closes window
		if (ImGui::Button("Save and Close")) {
			if (settingsManager.SaveAllSettings()) {
				std::unordered_map<std::string, std::any> eventData;
				eventData["workspaceID"] = GetID();
				eventData["viewTypeName"] = viewName;
				ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
			}
		}

		// Only show other buttons if there are changes
		if (settingsManager.HasAnyUnsavedChanges()) {
			ImGui::SameLine();
			if (ImGui::Button("Cancel Changes")) {
				settingsManager.RestoreAllFromBackups();
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset to Defaults")) {
				settingsManager.ResetAllToDefaults();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Close")) {
			HandleWindowClose();
		}
	}

	void SettingsView::RenderSaveConfirmationPopup() {
		if (showSavePopup) {
			ImGui::OpenPopup("Settings Saved");
		}

		if (ImGui::BeginPopupModal("Settings Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("All settings have been saved successfully.");
			ImGui::Separator();
			if (ImGui::Button("OK")) {
				showSavePopup = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void SettingsView::RenderUnsavedChangesDialog() {
		if (showUnsavedChangesDialog) {
			ImGui::OpenPopup("Unsaved Changes");
		}

		if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("You have unsaved changes. What would you like to do?");
			ImGui::Separator();

			if (ImGui::Button("Apply and Close", ImVec2(120, 0))) {
				if (settingsManager.SaveAllSettings()) {
					showUnsavedChangesDialog = false;
					std::unordered_map<std::string, std::any> eventData;
					eventData["workspaceID"] = GetID();
					eventData["viewTypeName"] = viewName;
					ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
				settingsManager.RestoreAllFromBackups();
				showUnsavedChangesDialog = false;
				std::unordered_map<std::string, std::any> eventData;
				eventData["workspaceID"] = GetID();
				eventData["viewTypeName"] = viewName;
				ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				showUnsavedChangesDialog = false;
				pendingClose = false;
				windowOpen = true; // Re-open the window
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

} // namespace GUI