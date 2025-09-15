#pragma once

#include "BaseTabObject.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Settings {

	// Simple settings manager that only knows about BaseTabObject
	class SettingsManager {
	public:
		SettingsManager() = default;
		~SettingsManager() = default;

		// Plugin interface for adding custom tabs
		void RegisterTab(std::unique_ptr<BaseTabObject> tab);
		void UnregisterTab(const std::string& tabName);

		// Core functionality
		bool LoadAllSettings();
		bool SaveAllSettings();
		void ResetAllToDefaults();
		void CreateAllBackups();
		void RestoreAllFromBackups();
		bool HasAnyUnsavedChanges() const;

		// Tab access for SettingsView
		const std::vector<std::unique_ptr<BaseTabObject>>& GetTabs() const { return tabs; }
		std::vector<std::string> GetCategories() const;
		std::vector<BaseTabObject*> GetTabsInCategory(const std::string& category) const;

		// Active tab management
		void SetActiveTab(const std::string& tabName);
		void SetActiveCategory(const std::string& category);
		BaseTabObject* GetActiveTab() const;
		const std::string& GetActiveCategory() const { return activeCategory; }

	private:
		std::vector<std::unique_ptr<BaseTabObject>> tabs;
		std::string activeCategory = "All";
		std::string activeTabName;
	};

} // namespace Settings