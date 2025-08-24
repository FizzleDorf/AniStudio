#include "SettingsView.hpp"
#include "PathsSettings.hpp"
#include "GeneralSettings.hpp"
#include "SDCPPSettings.hpp"
#include "../events/Events.hpp"
#include <iostream>

namespace GUI {

	SettingsView::SettingsView(ECS::EntityManager &mgr)
		: BaseView(mgr), selectedCategory("All"), showSavePopup(false),
		showUnsavedChangesDialog(false), pendingClose(false), windowOpen(true) {
		viewName = "SettingsView";

		// Register core tabs
		RegisterCoreTabs();

		// Load all settings on startup
		settingsManager.LoadAllSettings();
		std::cout << "[SettingsView] Initialized" << std::endl;
	}

	void SettingsView::RegisterCoreTabs() {
		std::cout << "[SettingsView] Registering core tabs..." << std::endl;

		settingsManager.RegisterTab(std::make_unique<Settings::GeneralSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::PathsSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::SDCPPSettingsTab>());

		std::cout << "[SettingsView] Registered core tabs" << std::endl;
	}

	std::string SettingsView::GetWindowTitle() const {
		return viewName + "###SettingsWindow" + std::to_string(GetID());
	}

	void SettingsView::Render() {
		ImGui::SetNextWindowSize(ImVec2(900, 700), ImGuiCond_FirstUseEver);

		if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen)) {

			// Check if window was closed via X button
			if (!windowOpen) {
				HandleWindowClose();
			}
			else {
				// Normal rendering
				RenderMainContent();
			}

		}
		ImGui::End();

		// Render popups outside of the main window - these should always render
		RenderSaveConfirmationPopup();
		RenderUnsavedChangesDialog();
	}

	void SettingsView::RenderMainContent() {
		// Create horizontal layout with category filter on left
		if (ImGui::BeginTable("SettingsLayout", 2, ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Categories", ImGuiTableColumnFlags_WidthFixed, 180.0f);
			ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();

			// Left column - Category filter
			ImGui::TableNextColumn();
			RenderCategoryFilter();

			// Right column - Settings content
			ImGui::TableNextColumn();
			RenderSettingsContent();

			ImGui::EndTable();
		}

		ImGui::Separator();
		RenderActionButtons();
	}

	void SettingsView::HandleWindowClose() {
		if (settingsManager.HasAnyUnsavedChanges()) {
			showUnsavedChangesDialog = true;
			pendingClose = true;
			windowOpen = true; // Keep window open until user decides
		}
		else {
			// No unsaved changes, close immediately
			ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
		}
	}

	void SettingsView::RenderCategoryFilter() {
		ImGui::Text("Categories");
		ImGui::Separator();

		// "All" option at top
		bool isAllSelected = (selectedCategory == "All");
		if (ImGui::Selectable("All", isAllSelected)) {
			selectedCategory = "All";
			settingsManager.SetActiveCategory("All");
		}

		ImGui::Separator();

		// Get categories and render them
		std::vector<std::string> categories = settingsManager.GetCategories();

		for (const auto& category : categories) {
			bool isCategorySelected = (selectedCategory == category);
			if (ImGui::Selectable(category.c_str(), isCategorySelected)) {
				selectedCategory = category;
				settingsManager.SetActiveCategory(category);
			}
			ImGui::Separator();
		}
	}

	void SettingsView::RenderSettingsContent() {
		ImGui::BeginChild("SettingsContent", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		if (selectedCategory == "All") {
			// Render all tabs with category headers
			std::vector<std::string> categories = settingsManager.GetCategories();
			for (const auto& category : categories) {
				ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s Settings", category.c_str());

				auto tabsInCategory = settingsManager.GetTabsInCategory(category);
				for (auto* tab : tabsInCategory) {
					tab->RenderUI();
				}

				ImGui::Separator();
			}
		}
		else {
			// Render tabs in selected category
			auto tabsInCategory = settingsManager.GetTabsInCategory(selectedCategory);
			for (auto* tab : tabsInCategory) {
				tab->RenderUI();
			}
		}

		ImGui::EndChild();
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
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
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
					ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
				settingsManager.RestoreAllFromBackups();
				showUnsavedChangesDialog = false;
				ANI::Events::Ref().RequestRemoveView(GetID(), viewName);
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