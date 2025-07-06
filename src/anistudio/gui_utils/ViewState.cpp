#include "ViewState.hpp"
#include "BaseView.hpp"
#include "AllViews.h" // Include all view types
#include <iostream>
#include <fstream>
#include <algorithm>

namespace GUI {

	// ViewInstanceState implementation
	nlohmann::json ViewInstanceState::Serialize() const {
		nlohmann::json j;
		j["viewTypeName"] = viewTypeName;
		j["viewTypeID"] = viewTypeID;
		j["viewListID"] = viewListID;
		j["isOpen"] = isOpen;
		j["isVisible"] = isVisible;
		j["viewData"] = viewData;
		j["isDocked"] = isDocked;
		j["dockSpaceID"] = dockSpaceID;
		j["windowPos"] = { windowPos.x, windowPos.y };
		j["windowSize"] = { windowSize.x, windowSize.y };
		j["windowCollapsed"] = windowCollapsed;
		return j;
	}

	void ViewInstanceState::Deserialize(const nlohmann::json& j) {
		if (j.contains("viewTypeName")) viewTypeName = j["viewTypeName"];
		if (j.contains("viewTypeID")) viewTypeID = j["viewTypeID"];
		if (j.contains("viewListID")) viewListID = j["viewListID"];
		if (j.contains("isOpen")) isOpen = j["isOpen"];
		if (j.contains("isVisible")) isVisible = j["isVisible"];
		if (j.contains("viewData")) viewData = j["viewData"];
		if (j.contains("isDocked")) isDocked = j["isDocked"];
		if (j.contains("dockSpaceID")) dockSpaceID = j["dockSpaceID"];
		if (j.contains("windowPos") && j["windowPos"].is_array() && j["windowPos"].size() >= 2) {
			windowPos.x = j["windowPos"][0];
			windowPos.y = j["windowPos"][1];
		}
		if (j.contains("windowSize") && j["windowSize"].is_array() && j["windowSize"].size() >= 2) {
			windowSize.x = j["windowSize"][0];
			windowSize.y = j["windowSize"][1];
		}
		if (j.contains("windowCollapsed")) windowCollapsed = j["windowCollapsed"];
	}

	// WorkspaceState implementation
	nlohmann::json WorkspaceState::Serialize() const {
		nlohmann::json j;
		j["workspaceName"] = workspaceName;
		j["mainMenuBarVisible"] = mainMenuBarVisible;
		j["statusBarVisible"] = statusBarVisible;
		j["toolbarVisible"] = toolbarVisible;
		j["dockingLayoutData"] = dockingLayoutData;

		nlohmann::json viewStatesJson;
		for (const auto&[viewID, state] : viewStates) {
			viewStatesJson[std::to_string(viewID)] = state.Serialize();
		}
		j["viewStates"] = viewStatesJson;

		return j;
	}

	void WorkspaceState::Deserialize(const nlohmann::json& j) {
		if (j.contains("workspaceName")) workspaceName = j["workspaceName"];
		if (j.contains("mainMenuBarVisible")) mainMenuBarVisible = j["mainMenuBarVisible"];
		if (j.contains("statusBarVisible")) statusBarVisible = j["statusBarVisible"];
		if (j.contains("toolbarVisible")) toolbarVisible = j["toolbarVisible"];
		if (j.contains("dockingLayoutData")) dockingLayoutData = j["dockingLayoutData"];

		if (j.contains("viewStates") && j["viewStates"].is_object()) {
			viewStates.clear();
			for (const auto&[idStr, stateJson] : j["viewStates"].items()) {
				try {
					ViewListID viewID = std::stoull(idStr);
					ViewInstanceState state;
					state.Deserialize(stateJson);
					viewStates[viewID] = state;
				}
				catch (const std::exception& e) {
					std::cerr << "[WorkspaceState] Failed to deserialize view state for ID " << idStr << ": " << e.what() << std::endl;
				}
			}
		}
	}

	void WorkspaceState::AddViewState(const ViewInstanceState& state) {
		viewStates[state.viewListID] = state;
	}

	void WorkspaceState::RemoveViewState(ViewListID viewID) {
		viewStates.erase(viewID);
	}

	bool WorkspaceState::HasViewState(ViewListID viewID) const {
		return viewStates.find(viewID) != viewStates.end();
	}

	ViewInstanceState* WorkspaceState::GetViewState(ViewListID viewID) {
		auto it = viewStates.find(viewID);
		return (it != viewStates.end()) ? &it->second : nullptr;
	}

	const ViewInstanceState* WorkspaceState::GetViewState(ViewListID viewID) const {
		auto it = viewStates.find(viewID);
		return (it != viewStates.end()) ? &it->second : nullptr;
	}

	size_t WorkspaceState::GetOpenViewCount() const {
		return std::count_if(viewStates.begin(), viewStates.end(),
			[](const auto& pair) { return pair.second.isOpen; });
	}

	size_t WorkspaceState::GetTotalViewCount() const {
		return viewStates.size();
	}

	std::vector<ViewListID> WorkspaceState::GetOpenViews() const {
		std::vector<ViewListID> openViews;
		for (const auto&[viewID, state] : viewStates) {
			if (state.isOpen) {
				openViews.push_back(viewID);
			}
		}
		return openViews;
	}

	std::vector<ViewListID> WorkspaceState::GetAllViews() const {
		std::vector<ViewListID> allViews;
		for (const auto&[viewID, state] : viewStates) {
			allViews.push_back(viewID);
		}
		return allViews;
	}

	// ViewState implementation
	ViewState::ViewState() {
		CreateDefaultWorkspace();
	}

	bool ViewState::CreateWorkspace(const std::string& name) {
		if (!IsValidWorkspaceName(name)) {
			return false;
		}

		if (m_workspaces.find(name) != m_workspaces.end()) {
			return false; // Workspace already exists
		}

		WorkspaceState workspace;
		workspace.workspaceName = name;
		m_workspaces[name] = workspace;

		std::cout << "[ViewState] Created workspace: " << name << std::endl;
		return true;
	}

	bool ViewState::DeleteWorkspace(const std::string& name) {
		if (name == "Default") {
			return false; // Cannot delete default workspace
		}

		auto it = m_workspaces.find(name);
		if (it == m_workspaces.end()) {
			return false; // Workspace doesn't exist
		}

		// If deleting active workspace, switch to default
		if (m_activeWorkspaceName == name) {
			m_activeWorkspaceName = "Default";
		}

		m_workspaces.erase(it);
		std::cout << "[ViewState] Deleted workspace: " << name << std::endl;
		return true;
	}

	bool ViewState::SetActiveWorkspace(const std::string& name) {
		if (m_workspaces.find(name) == m_workspaces.end()) {
			return false; // Workspace doesn't exist
		}

		m_activeWorkspaceName = name;
		std::cout << "[ViewState] Switched to workspace: " << name << std::endl;
		return true;
	}

	std::vector<std::string> ViewState::GetWorkspaceNames() const {
		std::vector<std::string> names;
		for (const auto&[name, workspace] : m_workspaces) {
			names.push_back(name);
		}
		return names;
	}

	WorkspaceState* ViewState::GetActiveWorkspace() {
		auto it = m_workspaces.find(m_activeWorkspaceName);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	const WorkspaceState* ViewState::GetActiveWorkspace() const {
		auto it = m_workspaces.find(m_activeWorkspaceName);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	WorkspaceState* ViewState::GetWorkspace(const std::string& name) {
		auto it = m_workspaces.find(name);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	const WorkspaceState* ViewState::GetWorkspace(const std::string& name) const {
		auto it = m_workspaces.find(name);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	bool ViewState::CaptureCurrentState(ViewManager& viewManager) {
		try {
			WorkspaceState* activeWorkspace = GetActiveWorkspace();
			if (!activeWorkspace) {
				std::cerr << "[ViewState] No active workspace found" << std::endl;
				return false;
			}

			// Clear current view states
			activeWorkspace->viewStates.clear();

			// Get all current views from ViewManager
			const auto& viewSignatures = viewManager.GetViewSignatures();
			const auto& registeredViews = viewManager.GetRegisteredViews();

			// Create reverse lookup map (typeID -> typeName)
			std::unordered_map<ViewTypeID, std::string> typeIDToName;
			for (const auto&[name, typeID] : registeredViews) {
				typeIDToName[typeID] = name;
			}

			// Capture state for each view
			for (const auto&[viewListID, signature] : viewSignatures) {
				for (const auto& viewTypeID : *signature) {
					ViewInstanceState state;
					state.viewListID = viewListID;
					state.viewTypeID = viewTypeID;
					state.isOpen = true; // If it exists in signatures, it's open

					// Find view type name
					auto nameIt = typeIDToName.find(viewTypeID);
					if (nameIt != typeIDToName.end()) {
						state.viewTypeName = nameIt->second;

						// Try to capture view-specific data by calling the view's Serialize method
						try {
							// This is a bit tricky - we need to know the actual view type to call Serialize
							// For now, we'll store basic info and implement specific serialization later
							state.viewData = nlohmann::json{};

							// TODO: Add specific view serialization based on type
							if (state.viewTypeName == "DiffusionView") {
								if (viewManager.HasView<DiffusionView>(viewListID)) {
									auto& view = viewManager.GetView<DiffusionView>(viewListID);
									state.viewData = view.Serialize();
								}
							}
							else if (state.viewTypeName == "UpscaleView") {
								if (viewManager.HasView<UpscaleView>(viewListID)) {
									auto& view = viewManager.GetView<UpscaleView>(viewListID);
									state.viewData = view.Serialize();
								}
							}
							// Add more view types as needed...

						}
						catch (const std::exception& e) {
							std::cerr << "[ViewState] Failed to serialize view " << state.viewTypeName << ": " << e.what() << std::endl;
							state.viewData = nlohmann::json{};
						}

						activeWorkspace->AddViewState(state);
					}
				}
			}

			std::cout << "[ViewState] Captured state for " << activeWorkspace->viewStates.size() << " views" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to capture current state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::RestoreState(ViewManager& viewManager, ECS::EntityManager& entityManager) {
		try {
			const WorkspaceState* activeWorkspace = GetActiveWorkspace();
			if (!activeWorkspace) {
				std::cerr << "[ViewState] No active workspace found" << std::endl;
				return false;
			}

			// First, clear all existing views
			viewManager.Reset();

			// Re-register view types (this should be done by the core system)
			// The ViewManager should already have registered view types

			std::cout << "[ViewState] Restoring " << activeWorkspace->viewStates.size() << " views..." << std::endl;

			// Restore each view
			for (const auto&[viewListID, state] : activeWorkspace->viewStates) {
				if (!state.isOpen) continue;

				try {
					// Create new view list with the same ID if possible
					ViewListID newViewID = viewManager.CreateView();

					// TODO: Implement specific view restoration based on type
					if (state.viewTypeName == "DiffusionView") {
						DiffusionView view(entityManager);
						viewManager.AddView<DiffusionView>(newViewID, std::move(view));

						// Deserialize view-specific data
						if (!state.viewData.empty()) {
							auto& restoredView = viewManager.GetView<DiffusionView>(newViewID);
							restoredView.Deserialize(state.viewData);
						}
					}
					else if (state.viewTypeName == "UpscaleView") {
						UpscaleView view(entityManager);
						viewManager.AddView<UpscaleView>(newViewID, std::move(view));

						if (!state.viewData.empty()) {
							auto& restoredView = viewManager.GetView<UpscaleView>(newViewID);
							restoredView.Deserialize(state.viewData);
						}
					}
					else if (state.viewTypeName == "DebugView") {
						DebugView view(entityManager);
						viewManager.AddView<DebugView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "SettingsView") {
						SettingsView view(entityManager);
						viewManager.AddView<SettingsView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "ImageView") {
						ImageView view(entityManager);
						viewManager.AddView<ImageView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "NodeGraphView") {
						NodeGraphView view(entityManager);
						viewManager.AddView<NodeGraphView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "SequencerView") {
						SequencerView view(entityManager);
						viewManager.AddView<SequencerView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "ConvertView") {
						ConvertView view(entityManager);
						viewManager.AddView<ConvertView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "NodeView") {
						NodeView view(entityManager);
						viewManager.AddView<NodeView>(newViewID, std::move(view));
					}
					else if (state.viewTypeName == "VideoView") {
						VideoView view(entityManager);
						viewManager.AddView<VideoView>(newViewID, std::move(view));
					}
					// Add more view types as needed...
					else {
						std::cerr << "[ViewState] Unknown view type: " << state.viewTypeName << std::endl;
						viewManager.DestroyView(newViewID);
						continue;
					}

					std::cout << "[ViewState] Restored view: " << state.viewTypeName << " with ID: " << newViewID << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[ViewState] Failed to restore view " << state.viewTypeName << ": " << e.what() << std::endl;
				}
			}

			std::cout << "[ViewState] Successfully restored workspace: " << activeWorkspace->workspaceName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to restore state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::SaveViewState(ViewListID viewID, const BaseView& view) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return false;

		ViewInstanceState* state = activeWorkspace->GetViewState(viewID);
		if (!state) return false;

		try {
			state->viewData = view.Serialize();
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to save view state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::LoadViewState(ViewListID viewID, BaseView& view) {
		const WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return false;

		const ViewInstanceState* state = activeWorkspace->GetViewState(viewID);
		if (!state || state->viewData.empty()) return false;

		try {
			view.Deserialize(state->viewData);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to load view state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::RemoveViewState(ViewListID viewID) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return false;

		activeWorkspace->RemoveViewState(viewID);
		return true;
	}

	void ViewState::OnViewCreated(ViewListID viewID, const std::string& viewTypeName, ViewTypeID viewTypeID) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return;

		ViewInstanceState state;
		state.viewListID = viewID;
		state.viewTypeName = viewTypeName;
		state.viewTypeID = viewTypeID;
		state.isOpen = true;
		state.isVisible = true;

		activeWorkspace->AddViewState(state);
		std::cout << "[ViewState] Registered new view: " << viewTypeName << " with ID: " << viewID << std::endl;
	}

	void ViewState::OnViewDestroyed(ViewListID viewID) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return;

		activeWorkspace->RemoveViewState(viewID);
		std::cout << "[ViewState] Unregistered view with ID: " << viewID << std::endl;
	}

	void ViewState::OnViewOpened(ViewListID viewID) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return;

		ViewInstanceState* state = activeWorkspace->GetViewState(viewID);
		if (state) {
			state->isOpen = true;
		}
	}

	void ViewState::OnViewClosed(ViewListID viewID) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (!activeWorkspace) return;

		ViewInstanceState* state = activeWorkspace->GetViewState(viewID);
		if (state) {
			state->isOpen = false;
		}
	}

	nlohmann::json ViewState::Serialize() const {
		nlohmann::json j;
		j["activeWorkspace"] = m_activeWorkspaceName;

		nlohmann::json workspacesJson;
		for (const auto&[name, workspace] : m_workspaces) {
			workspacesJson[name] = workspace.Serialize();
		}
		j["workspaces"] = workspacesJson;

		return j;
	}

	void ViewState::Deserialize(const nlohmann::json& j) {
		if (j.contains("activeWorkspace")) {
			m_activeWorkspaceName = j["activeWorkspace"];
		}

		if (j.contains("workspaces") && j["workspaces"].is_object()) {
			m_workspaces.clear();
			for (const auto&[name, workspaceJson] : j["workspaces"].items()) {
				WorkspaceState workspace;
				workspace.Deserialize(workspaceJson);
				m_workspaces[name] = workspace;
			}
		}

		EnsureDefaultWorkspace();
	}

	bool ViewState::SaveToFile(const std::string& filepath) const {
		try {
			nlohmann::json j = Serialize();
			std::ofstream file(filepath);
			if (!file.is_open()) {
				std::cerr << "[ViewState] Failed to open file for writing: " << filepath << std::endl;
				return false;
			}

			file << j.dump(4);
			file.close();

			std::cout << "[ViewState] Saved view state to: " << filepath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to save to file: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::LoadFromFile(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (!file.is_open()) {
				std::cerr << "[ViewState] Failed to open file for reading: " << filepath << std::endl;
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			Deserialize(j);

			std::cout << "[ViewState] Loaded view state from: " << filepath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to load from file: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::ValidateState() const {
		// Check if active workspace exists
		if (m_workspaces.find(m_activeWorkspaceName) == m_workspaces.end()) {
			return false;
		}

		// Check if default workspace exists
		if (m_workspaces.find("Default") == m_workspaces.end()) {
			return false;
		}

		return true;
	}

	void ViewState::CleanupInvalidStates() {
		// Remove any invalid view states or workspaces
		for (auto&[name, workspace] : m_workspaces) {
			// Could add validation logic here
		}
	}

	void ViewState::Reset() {
		m_workspaces.clear();
		m_activeWorkspaceName = "Default";
		CreateDefaultWorkspace();
	}

	void ViewState::CreateDefaultWorkspace() {
		if (m_workspaces.find("Default") == m_workspaces.end()) {
			WorkspaceState defaultWorkspace;
			defaultWorkspace.workspaceName = "Default";
			m_workspaces["Default"] = defaultWorkspace;
		}
		m_activeWorkspaceName = "Default";
	}

	size_t ViewState::GetTotalViewCount() const {
		size_t total = 0;
		for (const auto&[name, workspace] : m_workspaces) {
			total += workspace.GetTotalViewCount();
		}
		return total;
	}

	size_t ViewState::GetOpenViewCount() const {
		const WorkspaceState* activeWorkspace = GetActiveWorkspace();
		return activeWorkspace ? activeWorkspace->GetOpenViewCount() : 0;
	}

	void ViewState::PrintDebugInfo() const {
		std::cout << "[ViewState] Debug Info:" << std::endl;
		std::cout << "  Active Workspace: " << m_activeWorkspaceName << std::endl;
		std::cout << "  Total Workspaces: " << m_workspaces.size() << std::endl;

		for (const auto&[name, workspace] : m_workspaces) {
			std::cout << "  Workspace '" << name << "': " << workspace.GetOpenViewCount()
				<< "/" << workspace.GetTotalViewCount() << " views open" << std::endl;
		}
	}

	bool ViewState::IsValidWorkspaceName(const std::string& name) const {
		return !name.empty() && name.length() <= 64 &&
			name.find_first_of("\\/:*?\"<>|") == std::string::npos;
	}

	void ViewState::EnsureDefaultWorkspace() {
		if (m_workspaces.find("Default") == m_workspaces.end()) {
			CreateDefaultWorkspace();
		}

		if (m_workspaces.find(m_activeWorkspaceName) == m_workspaces.end()) {
			m_activeWorkspaceName = "Default";
		}
	}

	std::string ViewState::GetViewTypeName(ViewTypeID typeID, ViewManager& viewManager) const {
		const auto& registeredViews = viewManager.GetRegisteredViews();
		for (const auto&[name, id] : registeredViews) {
			if (id == typeID) {
				return name;
			}
		}
		return "Unknown";
	}

	ViewTypeID ViewState::GetViewTypeID(const std::string& typeName, ViewManager& viewManager) const {
		try {
			return viewManager.GetViewType(typeName);
		}
		catch (const std::exception&) {
			return 0; // Invalid type ID
		}
	}

} // namespace GUI