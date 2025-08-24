/*
 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 */

#pragma once

#include <string>
#include <imgui.h>

namespace Settings {

	class BaseTabObject {
	public:
		BaseTabObject(const std::string& name, const std::string& category)
			: tabName(name), categoryName(category) {}
		virtual ~BaseTabObject() = default;

		// Pure virtual functions that tabs must implement
		virtual void RenderUI() = 0;
		virtual bool SaveSettings() = 0;
		virtual bool LoadSettings() = 0;
		virtual void ResetToDefaults() = 0;
		virtual void CreateBackup() = 0;
		virtual void RestoreFromBackup() = 0;
		virtual bool HasUnsavedChanges() const = 0;

		// Getters
		const std::string& GetTabName() const { return tabName; }
		const std::string& GetCategoryName() const { return categoryName; }

	protected:
		std::string tabName;
		std::string categoryName;

		// Helper to get settings directory
		std::string GetSettingsDirectory() const {
			return "data/settings"; // Simple path, adjust as needed
		}
	};

} // namespace Settings