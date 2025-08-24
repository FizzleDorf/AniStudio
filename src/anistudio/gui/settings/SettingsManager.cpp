#include "SettingsManager.hpp"
#include <algorithm>
#include <iostream>

namespace Settings {

	void SettingsManager::RegisterTab(std::unique_ptr<BaseTabObject> tab) {
		std::cout << "[SettingsManager] Registering tab: " << tab->GetTabName() << " in category: " << tab->GetCategoryName() << std::endl;
		tabs.push_back(std::move(tab));
	}

	void SettingsManager::UnregisterTab(const std::string& tabName) {
		auto it = std::find_if(tabs.begin(), tabs.end(),
			[&tabName](const std::unique_ptr<BaseTabObject>& tab) {
			return tab->GetTabName() == tabName;
		});

		if (it != tabs.end()) {
			std::cout << "[SettingsManager] Unregistering tab: " << tabName << std::endl;
			tabs.erase(it);
		}
	}

	bool SettingsManager::LoadAllSettings() {
		std::cout << "[SettingsManager] Loading all settings..." << std::endl;
		bool success = true;

		for (auto& tab : tabs) {
			if (!tab->LoadSettings()) {
				std::cout << "[SettingsManager] Failed to load settings for: " << tab->GetTabName() << std::endl;
				success = false;
			}
			else {
				std::cout << "[SettingsManager] Loaded settings for: " << tab->GetTabName() << std::endl;
			}
		}

		return success;
	}

	bool SettingsManager::SaveAllSettings() {
		std::cout << "[SettingsManager] Saving all settings..." << std::endl;
		bool success = true;

		for (auto& tab : tabs) {
			if (!tab->SaveSettings()) {
				std::cout << "[SettingsManager] Failed to save settings for: " << tab->GetTabName() << std::endl;
				success = false;
			}
			else {
				std::cout << "[SettingsManager] Saved settings for: " << tab->GetTabName() << std::endl;
			}
		}

		if (success) {
			CreateAllBackups(); // Update backups after successful save
		}

		return success;
	}

	void SettingsManager::ResetAllToDefaults() {
		std::cout << "[SettingsManager] Resetting all settings to defaults..." << std::endl;

		for (auto& tab : tabs) {
			tab->ResetToDefaults();
			std::cout << "[SettingsManager] Reset to defaults: " << tab->GetTabName() << std::endl;
		}
	}

	void SettingsManager::CreateAllBackups() {
		for (auto& tab : tabs) {
			tab->CreateBackup();
		}
	}

	void SettingsManager::RestoreAllFromBackups() {
		std::cout << "[SettingsManager] Restoring all settings from backups..." << std::endl;

		for (auto& tab : tabs) {
			tab->RestoreFromBackup();
			std::cout << "[SettingsManager] Restored from backup: " << tab->GetTabName() << std::endl;
		}
	}

	bool SettingsManager::HasAnyUnsavedChanges() const {
		for (const auto& tab : tabs) {
			if (tab->HasUnsavedChanges()) {
				return true;
			}
		}
		return false;
	}

	std::vector<std::string> SettingsManager::GetCategories() const {
		std::vector<std::string> categories;

		for (const auto& tab : tabs) {
			const std::string& category = tab->GetCategoryName();
			if (std::find(categories.begin(), categories.end(), category) == categories.end()) {
				categories.push_back(category);
			}
		}

		return categories;
	}

	std::vector<BaseTabObject*> SettingsManager::GetTabsInCategory(const std::string& category) const {
		std::vector<BaseTabObject*> result;

		for (const auto& tab : tabs) {
			if (tab->GetCategoryName() == category) {
				result.push_back(tab.get());
			}
		}

		return result;
	}

	void SettingsManager::SetActiveTab(const std::string& tabName) {
		activeTabName = tabName;

		// Set active category based on tab
		for (const auto& tab : tabs) {
			if (tab->GetTabName() == tabName) {
				activeCategory = tab->GetCategoryName();
				break;
			}
		}
	}

	void SettingsManager::SetActiveCategory(const std::string& category) {
		activeCategory = category;

		// Clear active tab if switching categories
		if (category != "All") {
			activeTabName.clear();
		}
	}

	BaseTabObject* SettingsManager::GetActiveTab() const {
		if (activeTabName.empty()) return nullptr;

		for (const auto& tab : tabs) {
			if (tab->GetTabName() == activeTabName) {
				return tab.get();
			}
		}
		return nullptr;
	}

} // namespace Settings