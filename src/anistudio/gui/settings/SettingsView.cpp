#include "SettingsView.hpp"
#include "PathsSettings.hpp"
#include "GeneralSettings.hpp"
#include "SDCPPSettings.hpp"
#include "ImGuiStyleSettings.hpp"
#include "ImGuiRenderSettings.hpp"
#include "../events/Events.hpp"
#include <iostream>

namespace GUI {

	SettingsView::SettingsView(ECS::EntityManager &mgr)
		: BaseView(mgr), showSavePopup(false),
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
		settingsManager.RegisterTab(std::make_unique<Settings::ImGuiStyleSettingsTab>());
		settingsManager.RegisterTab(std::make_unique<Settings::ImGuiRenderSettingsTab>());

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
		// Create tabs for each BaseTabObject
		if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {

			// Iterate through all registered tabs and create ImGui tabs
			const auto& tabs = settingsManager.GetTabs();
			for (const auto& tab : tabs) {
				std::string tabTitle = tab->GetTabName() + " Settings";

				if (ImGui::BeginTabItem(tabTitle.c_str())) {
					// Render the tab content in a scrollable child window
					ImGui::BeginChild("TabContent", ImVec2(0, -50), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

					// Let the tab object render its UI
					tab->RenderUI();

					ImGui::EndChild();
					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
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