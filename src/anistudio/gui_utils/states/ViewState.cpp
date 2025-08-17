#include "ViewState.hpp"
#include "ViewManager.hpp"
#include <fstream>
#include <iostream>

namespace GUI {

	bool ViewState::SaveViewManagerState(const ViewManager& viewManager, const std::string& filepath) const {
		try {
			nlohmann::json j = SerializeViewManagerState(viewManager);
			std::ofstream file(filepath);
			if (!file.is_open()) {
				std::cerr << "[ViewState] Failed to open file for writing: " << filepath << std::endl;
				return false;
			}

			file << j.dump(4);
			file.close();
			std::cout << "[ViewState] Saved ViewManager state to: " << filepath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to save ViewManager state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ViewState::LoadViewManagerState(ViewManager& viewManager, const std::string& filepath) {
		try {
			std::ifstream file(filepath);
			if (!file.is_open()) {
				std::cout << "[ViewState] ViewManager state file not found: " << filepath << " (will use defaults)" << std::endl;
				return false;
			}

			nlohmann::json j;
			file >> j;
			file.close();

			DeserializeViewManagerState(viewManager, j);
			std::cout << "[ViewState] Loaded ViewManager state from: " << filepath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ViewState] Failed to load ViewManager state: " << e.what() << std::endl;
			return false;
		}
	}

	nlohmann::json ViewState::SerializeViewManagerState(const ViewManager& viewManager) const {
		nlohmann::json j;

		// Save the last active workspace
		j["lastActiveWorkspace"] = m_lastActiveWorkspace;

		// Use ViewManager's serialization method to save all workspaces and views
		j["workspaces"] = viewManager.SerializeViewLists();

		std::cout << "[ViewState] Serialized ViewManager state with last active workspace: " << m_lastActiveWorkspace << std::endl;
		return j;
	}

	void ViewState::DeserializeViewManagerState(ViewManager& viewManager, const nlohmann::json& j) {
		std::cout << "[ViewState] Deserializing ViewManager state..." << std::endl;

		// Load workspace data using ViewManager's deserialization - NO RESET!
		if (j.contains("workspaces")) {
			std::cout << "[ViewState] Loading workspaces from saved data..." << std::endl;
			viewManager.DeserializeViewLists(j["workspaces"]);
		}

		// Load the last active workspace
		if (j.contains("lastActiveWorkspace")) {
			m_lastActiveWorkspace = j["lastActiveWorkspace"];
			std::cout << "[ViewState] Restored last active workspace: " << m_lastActiveWorkspace << std::endl;
		}

		// Validate that the last active workspace exists, if not create one
		auto allWorkspaces = viewManager.GetAllWorkspaces();
		if (allWorkspaces.empty()) {
			WorkspaceID defaultWorkspace = viewManager.CreateView();
			m_lastActiveWorkspace = defaultWorkspace;
			std::cout << "[ViewState] No workspaces loaded, created default workspace: " << defaultWorkspace << std::endl;
		}
		else if (std::find(allWorkspaces.begin(), allWorkspaces.end(), m_lastActiveWorkspace) == allWorkspaces.end()) {
			// Last active workspace doesn't exist, use the first available
			m_lastActiveWorkspace = allWorkspaces[0];
			std::cout << "[ViewState] Last active workspace invalid, using: " << m_lastActiveWorkspace << std::endl;
		}

		std::cout << "[ViewState] ViewManager state deserialization complete" << std::endl;
	}

	void ViewState::Reset() {
		std::cout << "[ViewState] Resetting to defaults" << std::endl;
		m_lastActiveWorkspace = 0;
	}

} // namespace GUI