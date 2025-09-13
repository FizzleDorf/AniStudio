// SettingsView.hpp
#pragma once

#include "GUI.h"
#include "SettingsManager.hpp"
#include "BaseTabObject.hpp"
#include <string>
#include <vector>
#include <set>

namespace GUI {

	class SettingsView : public BaseView {
	public:
		static constexpr const char* GetMetadataJSON() {
			return R"({
            "displayName": "Settings",
            "category": "Edit",
            "description": "Manage application and project settings."
        })";
		}

		SettingsView(ECS::EntityManager &mgr);
		virtual ~SettingsView() = default;

		void Render() override;

		// Plugin interface - expose manager for plugin registration
		Settings::SettingsManager& GetSettingsManager() { return settingsManager; }

		// Get proper window title like HelpView
		std::string GetWindowTitle() const;

	private:
		// Settings manager owned by this view
		Settings::SettingsManager settingsManager;

		// Dialog state
		bool showSavePopup;
		bool showUnsavedChangesDialog;
		bool pendingClose;
		bool windowOpen;

		// Filter list state
		float filterListWidth;
		std::set<std::string> availableCategories;
		std::set<std::string> selectedCategories;
		std::string lastSelectedCategory;

		// Register core tabs
		void RegisterCoreTabs();

		// Handle window close attempts
		void HandleWindowClose();

		// UI rendering methods
		void RenderMainContent();
		void RenderFilterList();
		void RenderSelectedCategoriesContent();
		void RenderActionButtons();
		void RenderSaveConfirmationPopup();
		void RenderUnsavedChangesDialog();

		// Helper methods for dynamic category filtering
		void UpdateCategoriesForActiveTab(const std::string& activeTabName);
		std::vector<std::string> GetCategoriesFromTab(Settings::BaseTabObject* tab);
		void RenderFilteredTabContent(Settings::BaseTabObject* tab, const std::set<std::string>& selectedCategories);
		void HandleCategorySelection(const std::string& categoryName, bool ctrlHeld, bool shiftHeld);
		void SelectAllCategories();
		void DeselectAll();
	};

} // namespace GUI