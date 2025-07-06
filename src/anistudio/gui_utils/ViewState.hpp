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
#include "ViewTypes.hpp"
#include "ViewManager.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <memory>

namespace GUI {

	// Forward declarations
	class ViewManager;
	class BaseView;

	/**
	 * Individual view state information
	 */
	struct ViewInstanceState {
		std::string viewTypeName;
		ViewTypeID viewTypeID = 0;
		ViewListID viewListID = 0;
		bool isOpen = false;
		bool isVisible = true;
		nlohmann::json viewData; // Serialized view-specific data

		// Window/docking information
		bool isDocked = false;
		std::string dockSpaceID;
		ImVec2 windowPos = ImVec2(0, 0);
		ImVec2 windowSize = ImVec2(400, 300);
		bool windowCollapsed = false;

		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);
	};

	/**
	 * Complete workspace view state
	 */
	struct WorkspaceState {
		std::string workspaceName = "Default";
		std::unordered_map<ViewListID, ViewInstanceState> viewStates;

		// Workspace-level settings
		bool mainMenuBarVisible = true;
		bool statusBarVisible = true;
		bool toolbarVisible = true;

		// Docking layout
		std::string dockingLayoutData; // ImGui docking layout serialized as string

		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);

		// Helper methods
		void AddViewState(const ViewInstanceState& state);
		void RemoveViewState(ViewListID viewID);
		bool HasViewState(ViewListID viewID) const;
		ViewInstanceState* GetViewState(ViewListID viewID);
		const ViewInstanceState* GetViewState(ViewListID viewID) const;

		// Statistics
		size_t GetOpenViewCount() const;
		size_t GetTotalViewCount() const;
		std::vector<ViewListID> GetOpenViews() const;
		std::vector<ViewListID> GetAllViews() const;
	};

	/**
	 * Project-level view state management
	 * Handles multiple workspaces and view state persistence
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

		// View state capture and restoration
		bool CaptureCurrentState(ViewManager& viewManager);
		bool RestoreState(ViewManager& viewManager, ECS::EntityManager& entityManager);

		// Individual view management
		bool SaveViewState(ViewListID viewID, const BaseView& view);
		bool LoadViewState(ViewListID viewID, BaseView& view);
		bool RemoveViewState(ViewListID viewID);

		// View creation/destruction tracking
		void OnViewCreated(ViewListID viewID, const std::string& viewTypeName, ViewTypeID viewTypeID);
		void OnViewDestroyed(ViewListID viewID);
		void OnViewOpened(ViewListID viewID);
		void OnViewClosed(ViewListID viewID);

		// Serialization
		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);

		// File operations
		bool SaveToFile(const std::string& filepath) const;
		bool LoadFromFile(const std::string& filepath);

		// State validation
		bool ValidateState() const;
		void CleanupInvalidStates();

		// Reset to defaults
		void Reset();
		void CreateDefaultWorkspace();

		// Statistics and debugging
		size_t GetTotalViewCount() const;
		size_t GetOpenViewCount() const;
		void PrintDebugInfo() const;

	private:
		std::unordered_map<std::string, WorkspaceState> m_workspaces;
		std::string m_activeWorkspaceName = "Default";

		// Helper methods
		bool IsValidWorkspaceName(const std::string& name) const;
		void EnsureDefaultWorkspace();

		// View type name resolution (for serialization)
		std::string GetViewTypeName(ViewTypeID typeID, ViewManager& viewManager) const;
		ViewTypeID GetViewTypeID(const std::string& typeName, ViewManager& viewManager) const;
	};

} // namespace GUI