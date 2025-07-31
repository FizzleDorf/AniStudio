#include "ViewState.hpp"
#include <fstream>
#include <iostream>

namespace GUI {

	// WorkspaceState implementation
	nlohmann::json WorkspaceState::Serialize() const {
		nlohmann::json j;
		j["workspaceID"] = workspaceID;
		j["alias"] = alias;
		j["templateName"] = templateName;
		j["openViewTypes"] = std::vector<std::string>(openViewTypes.begin(), openViewTypes.end());
		j["mainMenuBarVisible"] = mainMenuBarVisible;
		j["statusBarVisible"] = statusBarVisible;
		j["toolbarVisible"] = toolbarVisible;
		return j;
	}

	void WorkspaceState::Deserialize(const nlohmann::json& j) {
		if (j.contains("workspaceID")) workspaceID = j["workspaceID"];
		if (j.contains("alias")) alias = j["alias"];
		if (j.contains("templateName")) templateName = j["templateName"];
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
		EnsureDefaultWorkspace();
	}

	size_t ViewState::CreateWorkspace(const std::string& templateName, const std::vector<std::string>& defaultViews) {
		size_t workspaceID = m_nextWorkspaceID++;

		WorkspaceState workspace;
		workspace.workspaceID = workspaceID;
		workspace.templateName = templateName;
		workspace.alias = GenerateUniqueAlias(templateName, workspaceID);

		// Add default views
		for (const auto& viewType : defaultViews) {
			workspace.openViewTypes.insert(viewType);
		}

		m_workspaces[workspaceID] = workspace;

		// Set as active if it's the first workspace
		if (m_activeWorkspaceID == 0) {
			m_activeWorkspaceID = workspaceID;
			std::cout << "[ViewState] Set initial active workspace to ID: " << workspaceID << std::endl;
		}

		std::cout << "[ViewState] Created workspace '" << workspace.alias << "' (ID: " << workspaceID << ") from template '" << templateName << "'" << std::endl;
		return workspaceID;
	}

	bool ViewState::DeleteWorkspace(size_t workspaceID) {
		if (workspaceID == 0) return false; // Can't delete invalid workspace

		auto it = m_workspaces.find(workspaceID);
		if (it == m_workspaces.end()) return false;

		// Don't allow deleting the last workspace
		if (m_workspaces.size() <= 1) return false;

		// If deleting active workspace, switch to another one
		if (m_activeWorkspaceID == workspaceID) {
			for (const auto&[id, workspace] : m_workspaces) {
				if (id != workspaceID) {
					m_activeWorkspaceID = id;
					std::cout << "[ViewState] Switched active workspace to ID: " << id << " (after deleting " << workspaceID << ")" << std::endl;
					break;
				}
			}
		}

		m_workspaces.erase(it);
		std::cout << "[ViewState] Deleted workspace ID: " << workspaceID << std::endl;
		return true;
	}

	bool ViewState::SetActiveWorkspace(size_t workspaceID) {
		if (m_workspaces.find(workspaceID) == m_workspaces.end()) {
			std::cerr << "[ViewState] Cannot set active workspace - ID " << workspaceID << " does not exist" << std::endl;
			return false;
		}

		// Only log and update if actually different
		if (m_activeWorkspaceID != workspaceID) {
			size_t oldActiveID = m_activeWorkspaceID;
			m_activeWorkspaceID = workspaceID;
			std::cout << "[ViewState] Changed active workspace from ID: " << oldActiveID << " to ID: " << workspaceID << std::endl;
		}

		return true;
	}

	bool ViewState::RenameWorkspace(size_t workspaceID, const std::string& newAlias) {
		auto it = m_workspaces.find(workspaceID);
		if (it == m_workspaces.end()) return false;

		std::string uniqueAlias = GenerateUniqueAlias(newAlias, workspaceID);
		std::string oldAlias = it->second.alias;
		it->second.alias = uniqueAlias;

		std::cout << "[ViewState] Renamed workspace " << workspaceID << " from '" << oldAlias << "' to '" << uniqueAlias << "'" << std::endl;
		return true;
	}

	std::string ViewState::GetWorkspaceAlias(size_t workspaceID) const {
		auto it = m_workspaces.find(workspaceID);
		return (it != m_workspaces.end()) ? it->second.alias : "";
	}

	size_t ViewState::GetWorkspaceByAlias(const std::string& alias) const {
		for (const auto&[id, workspace] : m_workspaces) {
			if (workspace.alias == alias) {
				return id;
			}
		}
		return 0; // Not found
	}

	std::vector<size_t> ViewState::GetWorkspaceIDs() const {
		std::vector<size_t> ids;
		for (const auto&[id, workspace] : m_workspaces) {
			ids.push_back(id);
		}
		return ids;
	}

	std::vector<std::pair<size_t, std::string>> ViewState::GetWorkspaceList() const {
		std::vector<std::pair<size_t, std::string>> list;
		for (const auto&[id, workspace] : m_workspaces) {
			list.emplace_back(id, workspace.alias);
		}
		return list;
	}

	WorkspaceState* ViewState::GetActiveWorkspace() {
		auto it = m_workspaces.find(m_activeWorkspaceID);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	const WorkspaceState* ViewState::GetActiveWorkspace() const {
		auto it = m_workspaces.find(m_activeWorkspaceID);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	WorkspaceState* ViewState::GetWorkspace(size_t workspaceID) {
		auto it = m_workspaces.find(workspaceID);
		return (it != m_workspaces.end()) ? &it->second : nullptr;
	}

	const WorkspaceState* ViewState::GetWorkspace(size_t workspaceID) const {
		auto it = m_workspaces.find(workspaceID);
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
			std::cout << "[ViewState] Set view '" << viewType << "' to " << (open ? "open" : "closed")
				<< " in workspace " << activeWorkspace->workspaceID << " (" << activeWorkspace->alias << ")" << std::endl;
		}
		else {
			std::cerr << "[ViewState] Cannot set view open - no active workspace!" << std::endl;
		}
	}

	void ViewState::ToggleView(const std::string& viewType) {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (activeWorkspace) {
			bool wasOpen = activeWorkspace->IsViewOpen(viewType);
			activeWorkspace->ToggleView(viewType);
			std::cout << "[ViewState] Toggled view '" << viewType << "' from " << (wasOpen ? "open" : "closed")
				<< " to " << (wasOpen ? "closed" : "open") << " in workspace " << activeWorkspace->workspaceID
				<< " (" << activeWorkspace->alias << ")" << std::endl;
		}
		else {
			std::cerr << "[ViewState] Cannot toggle view - no active workspace!" << std::endl;
		}
	}

	void ViewState::CloseAllViews() {
		WorkspaceState* activeWorkspace = GetActiveWorkspace();
		if (activeWorkspace) {
			activeWorkspace->openViewTypes.clear();
			std::cout << "[ViewState] Closed all views in workspace " << activeWorkspace->workspaceID
				<< " (" << activeWorkspace->alias << ")" << std::endl;
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

		// Always save the active workspace ID
		j["activeWorkspaceID"] = m_activeWorkspaceID;
		j["nextWorkspaceID"] = m_nextWorkspaceID;

		nlohmann::json workspacesJson = nlohmann::json::array();
		for (const auto&[id, workspace] : m_workspaces) {
			workspacesJson.push_back(workspace.Serialize());
		}
		j["workspaces"] = workspacesJson;

		std::cout << "[ViewState] Serialized ViewState with active workspace ID: " << m_activeWorkspaceID << std::endl;
		return j;
	}

	void ViewState::Deserialize(const nlohmann::json& j) {
		// Load workspace data first
		if (j.contains("nextWorkspaceID")) {
			m_nextWorkspaceID = j["nextWorkspaceID"];
		}

		if (j.contains("workspaces") && j["workspaces"].is_array()) {
			m_workspaces.clear();
			for (const auto& workspaceJson : j["workspaces"]) {
				WorkspaceState workspace;
				workspace.Deserialize(workspaceJson);
				m_workspaces[workspace.workspaceID] = workspace;
			}
		}

		// Load the active workspace ID AFTER workspaces are loaded
		if (j.contains("activeWorkspaceID")) {
			size_t savedActiveID = j["activeWorkspaceID"];

			// Verify the saved active workspace exists
			if (m_workspaces.find(savedActiveID) != m_workspaces.end()) {
				m_activeWorkspaceID = savedActiveID;
				std::cout << "[ViewState] Restored active workspace ID: " << m_activeWorkspaceID << std::endl;
			}
			else {
				std::cerr << "[ViewState] Saved active workspace ID " << savedActiveID << " not found, using fallback" << std::endl;
				EnsureDefaultWorkspace(); // This will set a valid active workspace
			}
		}
		else {
			std::cout << "[ViewState] No active workspace ID in saved data, using fallback" << std::endl;
			EnsureDefaultWorkspace();
		}

		// Final validation
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
			std::cout << "[ViewState] Saved ViewState to: " << filepath << " (active workspace: " << m_activeWorkspaceID << ")" << std::endl;
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
				std::cout << "[ViewState] File not found: " << filepath << " (will use defaults)" << std::endl;
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			Deserialize(j);
			std::cout << "[ViewState] Loaded ViewState from: " << filepath << " (active workspace: " << m_activeWorkspaceID << ")" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to load: " << e.what() << std::endl;
			return false;
		}
	}

	void ViewState::Reset() {
		std::cout << "[ViewState] Resetting to defaults" << std::endl;
		m_workspaces.clear();
		m_activeWorkspaceID = 0;
		m_nextWorkspaceID = 1;
		EnsureDefaultWorkspace();
	}

	std::string ViewState::GenerateUniqueAlias(const std::string& baseName, size_t workspaceID) {
		std::string alias = baseName;

		// Check if this exact alias exists (excluding our own workspace)
		bool exists = false;
		for (const auto&[id, workspace] : m_workspaces) {
			if (id != workspaceID && workspace.alias == alias) {
				exists = true;
				break;
			}
		}

		if (exists) {
			alias = baseName + "_" + std::to_string(workspaceID);
		}

		return alias;
	}

	void ViewState::EnsureDefaultWorkspace() {
		// Create default workspace if none exist
		if (m_workspaces.empty()) {
			std::cout << "[ViewState] No workspaces found, creating default" << std::endl;
			CreateWorkspace("Default", {});
		}

		// Make sure active workspace exists and is valid
		if (m_workspaces.find(m_activeWorkspaceID) == m_workspaces.end()) {
			if (!m_workspaces.empty()) {
				size_t oldActiveID = m_activeWorkspaceID;
				m_activeWorkspaceID = m_workspaces.begin()->first;
				std::cout << "[ViewState] Active workspace ID " << oldActiveID << " invalid, set to " << m_activeWorkspaceID << std::endl;
			}
			else {
				std::cerr << "[ViewState] Critical error: No workspaces available!" << std::endl;
			}
		}
	}

	bool ViewState::IsValidWorkspaceName(const std::string& name) const {
		return !name.empty() && name.length() <= 64 &&
			name.find_first_of("\\/:*?\"<>|") == std::string::npos;
	}

} // namespace GUI