#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <memory>

namespace Settings {

	struct FilterItem {
		std::string name;
		std::string category;
		bool isCategory;

		FilterItem(const std::string& n, const std::string& cat = "", bool isCat = false)
			: name(n), category(cat), isCategory(isCat) {}
	};

	class BaseTabObject {
	public:
		BaseTabObject(const std::string& name, const std::string& category)
			: tabName(name), tabCategory(category) {}

		virtual ~BaseTabObject() = default;

		// Pure virtual functions for derived classes
		virtual void RenderUI() = 0;
		virtual bool SaveSettings() = 0;
		virtual bool LoadSettings() = 0;
		virtual void ResetToDefaults() = 0;
		virtual void CreateBackup() = 0;
		virtual void RestoreFromBackup() = 0;
		virtual bool HasUnsavedChanges() const = 0;

		// Virtual method for plugins to override and provide their own categories
		virtual std::vector<std::string> GetCategories() const {
			// Default implementation returns empty - core tabs use hardcoded mapping
			// Plugin tabs should override this method
			return {};
		}

		// Virtual method for filtered rendering - plugins can override this
		virtual void RenderFilteredUI(const std::set<std::string>& selectedCategories) {
			// Default implementation just renders everything
			// Plugins can override to respect category filtering
			RenderUI();
		}

		// Getters
		const std::string& GetTabName() const { return tabName; }
		const std::string& GetTabCategory() const { return tabCategory; }

		// Tab ordering support for customizable order
		virtual int GetDisplayOrder() const { return 100; } // Default order, lower = earlier
		virtual void SetDisplayOrder(int order) { displayOrder = order; }

	protected:
		std::string tabName;
		std::string tabCategory;
		int displayOrder = 100;

		std::string GetSettingsDirectory() const {
			return "../data/settings";
		}

		std::string GetDefaultsDirectory() const {
			return "../data/defaults";
		}

		// Helper method for tabs to check if a category should be rendered
		bool ShouldRenderCategory(const std::string& categoryName, const std::set<std::string>& selectedCategories) const {
			return selectedCategories.empty() || selectedCategories.count(categoryName) > 0;
		}

	public:
		// Static comparison function for tab ordering
		static bool CompareTabOrder(const std::unique_ptr<BaseTabObject>& a, const std::unique_ptr<BaseTabObject>& b) {
			int orderA = a->GetDisplayOrder();
			int orderB = b->GetDisplayOrder();
			if (orderA != orderB) {
				return orderA < orderB;
			}
			// If same order, sort alphabetically
			return a->GetTabName() < b->GetTabName();
		}
	};

} // namespace Settings