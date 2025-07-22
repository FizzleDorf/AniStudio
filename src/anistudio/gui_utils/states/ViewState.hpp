/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 */

#pragma once
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace GUI {

	/**
	 * Simple workspace state - just tracks which view types should be open
	 */
	struct WorkspaceState {
		std::string workspaceName = "Default";
		std::unordered_set<std::string> openViewTypes; // Just track which view types are open

		// Workspace-level UI settings
		bool mainMenuBarVisible = true;
		bool statusBarVisible = true;
		bool toolbarVisible = true;

		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);

		// Simple helpers
		bool IsViewOpen(const std::string& viewType) const;
		void SetViewOpen(const std::string& viewType, bool open);
		void ToggleView(const std::string& viewType);
		size_t GetOpenViewCount() const { return openViewTypes.size(); }
		std::vector<std::string> GetOpenViews() const;
	};

	/**
	 * Simplified ViewState - only manages which views should be open
	 * Doesn't know about ViewManager or actual view instances
	 */
	class ViewState {
	public:
		ViewState();
		~ViewState() = default;

		// Workspace management
		bool CreateWorkspace(const std::string& name);
		bool DeleteWorkspace(const std::string& name);
		bool SetActiveWorkspace(const std::string& name);
		const std::string& GetActiveWorkspaceName() const { return m_activeWorkspaceName; }
		std::vector<std::string> GetWorkspaceNames() const;

		WorkspaceState* GetActiveWorkspace();
		const WorkspaceState* GetActiveWorkspace() const;
		WorkspaceState* GetWorkspace(const std::string& name);
		const WorkspaceState* GetWorkspace(const std::string& name) const;

		// View state management (delegates to active workspace)
		bool IsViewOpen(const std::string& viewType) const;
		void SetViewOpen(const std::string& viewType, bool open);
		void ToggleView(const std::string& viewType);
		void CloseAllViews();

		std::vector<std::string> GetOpenViewTypes() const;
		size_t GetOpenViewCount() const;

		// Serialization
		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);

		// File operations
		bool SaveToFile(const std::string& filepath) const;
		bool LoadFromFile(const std::string& filepath);

		// Reset to defaults
		void Reset();
		void CreateDefaultWorkspace();

		// Template application
		void ApplyTemplate(const std::vector<std::string>& viewTypes);

	private:
		std::unordered_map<std::string, WorkspaceState> m_workspaces;
		std::string m_activeWorkspaceName = "Default";

		void EnsureDefaultWorkspace();
		bool IsValidWorkspaceName(const std::string& name) const;
	};

} // namespace GUI