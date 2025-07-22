#include "ViewState.hpp"
#include <fstream>
#include <iostream>

namespace GUI {

	// WorkspaceState implementation
	nlohmann::json WorkspaceState::Serialize() const {
		nlohmann::json j;
		j["workspaceName"] = workspaceName;
		j["openViewTypes"] = std::vector<std::string>(openViewTypes.begin(), openViewTypes.end());
		j["mainMenuBarVisible"] = mainMenuBarVisible;
		j["statusBarVisible"] = statusBarVisible;
		j["toolbarVisible"] = toolbarVisible;
		return j;
	}

	void WorkspaceState::Deserialize(const nlohmann::json& j) {
		if (j.contains("workspaceName")) workspaceName = j["workspaceName"];
		if (j.contains("mainMenuBarVisible")) mainMenuBarVisible = j["mainMenuBarVisible"];
		if (j.contains("statusBarVisible")) statusBarVisible = j["statusBarVisible"];
		if (j.contains("toolbarVisible")) toolbarVisible = j["toolbarVisible"];

		if (j.contains("openViewTypes") && j["openViewTypes"].is_array()) {
			openViewTypes.clear();
			for (const auto& viewType : j["openViewTypes"]) {
				if (viewType.is_string()) {
					openViewTypes.insert(viewType.get<std::string>());
				}
			}
		}
	}

	bool WorkspaceState::IsViewOpen(const std::string& viewType) const {
		return openViewTypes.find(viewType) != openViewTypes.end();
	}

	void WorkspaceState::SetViewOpen(const std::string& viewType, bool open) {
		if (open) {
			openViewTypes.insert(viewType);
		}
		else {
			openViewTypes.erase(viewType);
		}
	}

	void WorkspaceState::ToggleView(const std::string& viewType) {
		if (IsViewOpen(viewType)) {
			openViewTypes.erase(viewType);
		}
		else {
			openViewTypes.insert(viewType);
		}
	}

	std::vector<std::string> WorkspaceState::GetOpenViews() const {
		return std::vector<std::string>(openViewTypes.begin(), openViewTypes.end());
	}

	// ViewState implementation
	ViewState::ViewState() {
		CreateDefaultWorkspace();
	}

	bool ViewState::CreateWorkspace(const std::string& name) {
		if (!IsValidWorkspaceName(name) || m_workspaces.find(name) != m_workspaces.end()) {
			return false;
		}

		WorkspaceState workspace;
		workspace.workspaceName = name;
		m_workspaces[name] = workspace;
		return true;
	}

	bool ViewState::DeleteWorkspace(const std::string& name) {
		if (name == "Default") {
			return false; // Cannot delete default workspace
		}

		auto it = m_workspaces.find(name);
		if (it == m_workspaces.end()) {
			return false;
		}

		// If deleting active workspace, switch to default
		if (m_activeWorkspaceName == name) {
			m_activeWorkspaceName = "Default";
		}

		m_workspaces.erase(it);
		return true;
	}

	bool ViewState::SetActiveWorkspace(const std::string& name) {
		if (m_workspaces.find(name) == m_workspaces.end()) {
			return false;
		}

		m_activeWorkspaceName = name;
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

	bool ViewState::IsViewOpen(const std::string& viewType) const {
		const WorkspaceState* activeWorkspace = GetActiveWorkspace();
		return activeWorkspace ? activeWorkspace->IsViewOpen(viewType) : false;
	}

	void ViewState::SetViewOpen(const std::string& viewType, bool open) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (activeWorkspace) {
			activeWorkspace->SetViewOpen(viewType, open);
		}
	}

	void ViewState::ToggleView(const std::string& viewType) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (activeWorkspace) {
			activeWorkspace->ToggleView(viewType);
		}
	}

	void ViewState::CloseAllViews() {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (activeWorkspace) {
			activeWorkspace->openViewTypes.clear();
		}
	}

	std::vector<std::string> ViewState::GetOpenViewTypes() const {
		const WorkspaceState* activeWorkspace = GetActiveWorkspace();
		return activeWorkspace ? activeWorkspace->GetOpenViews() : std::vector<std::string>{};
	}

	size_t ViewState::GetOpenViewCount() const {
		const WorkspaceState* activeWorkspace = GetActiveWorkspace();
		return activeWorkspace ? activeWorkspace->GetOpenViewCount() : 0;
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
				return false;
			}

			file << j.dump(4);
			file.close();
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to save: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::LoadFromFile(const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (!file.is_open()) {
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			Deserialize(j);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to load: " << e.what() << std::endl;
			return false;
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

	void ViewState::ApplyTemplate(const std::vector<std::string>& viewTypes) {
		CloseAllViews();
		for (const auto& viewType : viewTypes) {
			SetViewOpen(viewType, true);
		}
	}

	void ViewState::EnsureDefaultWorkspace() {
		if (m_workspaces.find("Default") == m_workspaces.end()) {
			CreateDefaultWorkspace();
		}

		if (m_workspaces.find(m_activeWorkspaceName) == m_workspaces.end()) {
			m_activeWorkspaceName = "Default";
		}
	}

	bool ViewState::IsValidWorkspaceName(const std::string& name) const {
		return !name.empty() && name.length() <= 64 &&
			name.find_first_of("\\/:*?\"<>|") == std::string::npos;
	}

} // namespace GUI