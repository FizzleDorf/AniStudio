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
	 * Workspace state with ID and alias management
	 */
	struct WorkspaceState {
		size_t workspaceID = 0;
		std::string alias = "Default";
		std::string templateName = "Blank";
		std::unordered_set<std::string> openViewTypes; // Which view types are open

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
	 * Enhanced ViewState - manages workspaces with IDs and aliases
	 */
	class ViewState {
	public:
		ViewState();
		~ViewState() = default;

		// Workspace management with IDs and aliases
		size_t CreateWorkspace(const std::string& templateName, const std::vector<std::string>& defaultViews);
		bool DeleteWorkspace(size_t workspaceID);
		bool SetActiveWorkspace(size_t workspaceID);
		size_t GetActiveWorkspaceID() const { return m_activeWorkspaceID; }

		// Alias management
		bool RenameWorkspace(size_t workspaceID, const std::string& newAlias);
		std::string GetWorkspaceAlias(size_t workspaceID) const;
		size_t GetWorkspaceByAlias(const std::string& alias) const;

		// Workspace queries
		std::vector<size_t> GetWorkspaceIDs() const;
		std::vector<std::pair<size_t, std::string>> GetWorkspaceList() const; // ID, alias pairs

		WorkspaceState* GetActiveWorkspace();
		const WorkspaceState* GetActiveWorkspace() const;
		WorkspaceState* GetWorkspace(size_t workspaceID);
		const WorkspaceState* GetWorkspace(size_t workspaceID) const;

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

	private:
		std::unordered_map<size_t, WorkspaceState> m_workspaces;
		size_t m_activeWorkspaceID = 0;
		size_t m_nextWorkspaceID = 1;

		std::string GenerateUniqueAlias(const std::string& baseName, size_t workspaceID);
		void EnsureDefaultWorkspace();
		bool IsValidWorkspaceName(const std::string& name) const;
	};

} // namespace GUI