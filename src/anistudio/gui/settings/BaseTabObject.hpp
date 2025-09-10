#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

namespace Settings {

	struct FilterItem {
		std::string name;
		std::string category;
		bool isCategory;

		FilterItem(const std::string& n, const std::string& cat = "", bool isCat = false)
			: name(n), category(cat), isCategory(isCat) {}
	};

	// Singleton to manage shared filter state
	class FilterManager {
	public:
		static FilterManager& Instance() {
			static FilterManager instance;
			return instance;
		}

		std::vector<FilterItem> availableFilters;
		std::set<std::string> selectedTabs;
		std::set<std::string> selectedCategories;
		std::string lastSelectedTab;

	private:
		FilterManager() = default;
	};

	class BaseTabObject {
	public:
		BaseTabObject(const std::string& name, const std::string& category)
			: tabName(name), tabCategory(category) {
			InitializeFilters();
		}

		virtual ~BaseTabObject() = default;

		// Pure virtual functions for derived classes
		virtual void RenderUI() = 0;
		virtual bool SaveSettings() = 0;
		virtual bool LoadSettings() = 0;
		virtual void ResetToDefaults() = 0;
		virtual void CreateBackup() = 0;
		virtual void RestoreFromBackup() = 0;
		virtual bool HasUnsavedChanges() const = 0;

		const std::string& GetTabName() const { return tabName; }
		const std::string& GetTabCategory() const { return tabCategory; }
		bool IsTabSelected() const { return FilterManager::Instance().selectedTabs.count(tabName) > 0; }

	protected:
		std::string tabName;
		std::string tabCategory;

		std::string GetSettingsDirectory() const {
			return "../data/settings";
		}

		std::string GetDefaultsDirectory() const {
			return "../data/defaults";
		}

	private:
		void InitializeFilters() {
			auto& manager = FilterManager::Instance();

			// Add this tab to available filters if not already present
			bool tabExists = false;
			bool categoryExists = false;

			for (const auto& filter : manager.availableFilters) {
				if (filter.name == tabName && !filter.isCategory) {
					tabExists = true;
				}
				if (filter.name == tabCategory && filter.isCategory) {
					categoryExists = true;
				}
			}

			if (!categoryExists && !tabCategory.empty()) {
				manager.availableFilters.emplace_back(tabCategory, "", true);
			}

			if (!tabExists) {
				manager.availableFilters.emplace_back(tabName, tabCategory, false);
			}

			// Sort filters by category then by name
			std::sort(manager.availableFilters.begin(), manager.availableFilters.end(),
				[](const FilterItem& a, const FilterItem& b) {
				if (a.isCategory && !b.isCategory) return true;
				if (!a.isCategory && b.isCategory) return false;
				if (a.category != b.category) return a.category < b.category;
				return a.name < b.name;
			});
		}

	public:
		static void RenderFilterList() {
			auto& manager = FilterManager::Instance();

			ImGui::Text("Settings Categories");
			ImGui::Separator();

			// Group filters by category
			std::map<std::string, std::vector<FilterItem*>> groupedFilters;

			// Initialize categories
			for (auto& filter : manager.availableFilters) {
				if (filter.isCategory) {
					groupedFilters[filter.name] = std::vector<FilterItem*>();
				}
			}

			// Add tabs to their categories
			for (auto& filter : manager.availableFilters) {
				if (!filter.isCategory && !filter.category.empty()) {
					if (groupedFilters.find(filter.category) != groupedFilters.end()) {
						groupedFilters[filter.category].push_back(&filter);
					}
				}
			}

			ImGuiIO& io = ImGui::GetIO();

			// Render categories and their tabs
			for (auto&[categoryName, tabs] : groupedFilters) {
				// Category header
				bool categorySelected = manager.selectedCategories.count(categoryName) > 0;

				ImGui::PushStyleColor(ImGuiCol_Text, categorySelected ?
					ImVec4(0.4f, 0.8f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

				bool categoryClicked = ImGui::Selectable(
					categoryName.c_str(),
					categorySelected,
					ImGuiSelectableFlags_AllowDoubleClick
				);

				ImGui::PopStyleColor();

				if (categoryClicked) {
					HandleCategorySelection(categoryName, io.KeyCtrl, io.KeyShift);
				}

				// Render tabs in this category (indented)
				for (auto* tab : tabs) {
					ImGui::Indent(20.0f);

					bool tabSelected = manager.selectedTabs.count(tab->name) > 0;
					ImGui::PushStyleColor(ImGuiCol_Text, tabSelected ?
						ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.9f, 0.9f, 0.9f, 1.0f));

					bool tabClicked = ImGui::Selectable(
						tab->name.c_str(),
						tabSelected,
						ImGuiSelectableFlags_AllowDoubleClick
					);

					ImGui::PopStyleColor();

					if (tabClicked) {
						HandleTabSelection(tab->name, io.KeyCtrl, io.KeyShift);
					}

					ImGui::Unindent(20.0f);
				}
			}

			ImGui::Separator();

			// Selection control buttons
			if (ImGui::Button("Select All", ImVec2(-1, 0))) {
				for (const auto& filter : manager.availableFilters) {
					if (!filter.isCategory) {
						manager.selectedTabs.insert(filter.name);
					}
					else {
						manager.selectedCategories.insert(filter.name);
					}
				}
			}

			if (ImGui::Button("Clear All", ImVec2(-1, 0))) {
				manager.selectedTabs.clear();
				manager.selectedCategories.clear();
			}

			ImGui::Text("Selected: %zu tabs", manager.selectedTabs.size());
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Ctrl+Click: Multi-select");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Shift+Click: Range select");
		}

		static void HandleCategorySelection(const std::string& categoryName, bool ctrlHeld, bool shiftHeld) {
			auto& manager = FilterManager::Instance();

			if (!ctrlHeld && !shiftHeld) {
				// Single selection - clear others and select this category's tabs
				manager.selectedTabs.clear();
				manager.selectedCategories.clear();
				manager.selectedCategories.insert(categoryName);

				// Select all tabs in this category
				for (const auto& filter : manager.availableFilters) {
					if (!filter.isCategory && filter.category == categoryName) {
						manager.selectedTabs.insert(filter.name);
					}
				}
			}
			else if (ctrlHeld) {
				// Toggle this category
				if (manager.selectedCategories.count(categoryName)) {
					manager.selectedCategories.erase(categoryName);
					// Remove tabs from this category
					for (const auto& filter : manager.availableFilters) {
						if (!filter.isCategory && filter.category == categoryName) {
							manager.selectedTabs.erase(filter.name);
						}
					}
				}
				else {
					manager.selectedCategories.insert(categoryName);
					// Add tabs from this category
					for (const auto& filter : manager.availableFilters) {
						if (!filter.isCategory && filter.category == categoryName) {
							manager.selectedTabs.insert(filter.name);
						}
					}
				}
			}
		}

		static void HandleTabSelection(const std::string& tabName, bool ctrlHeld, bool shiftHeld) {
			auto& manager = FilterManager::Instance();

			if (!ctrlHeld && !shiftHeld) {
				// Single selection
				manager.selectedTabs.clear();
				manager.selectedCategories.clear();
				manager.selectedTabs.insert(tabName);
				manager.lastSelectedTab = tabName;
			}
			else if (ctrlHeld) {
				// Toggle selection
				if (manager.selectedTabs.count(tabName)) {
					manager.selectedTabs.erase(tabName);
				}
				else {
					manager.selectedTabs.insert(tabName);
				}
				manager.lastSelectedTab = tabName;
			}
			else if (shiftHeld && !manager.lastSelectedTab.empty()) {
				// Range selection
				std::vector<std::string> tabOrder;
				for (const auto& filter : manager.availableFilters) {
					if (!filter.isCategory) {
						tabOrder.push_back(filter.name);
					}
				}

				auto startIt = std::find(tabOrder.begin(), tabOrder.end(), manager.lastSelectedTab);
				auto endIt = std::find(tabOrder.begin(), tabOrder.end(), tabName);

				if (startIt != tabOrder.end() && endIt != tabOrder.end()) {
					if (startIt > endIt) std::swap(startIt, endIt);

					for (auto it = startIt; it <= endIt; ++it) {
						manager.selectedTabs.insert(*it);
					}
				}
			}
		}
	};

} // namespace Settings