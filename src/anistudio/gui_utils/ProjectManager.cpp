#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ANI {

	// ProjectSettings implementation
	nlohmann::json ProjectSettings::Serialize() const {
		nlohmann::json j;
		j["projectName"] = projectName;
		j["projectVersion"] = projectVersion;
		j["projectDescription"] = projectDescription;
		j["createdBy"] = createdBy;
		j["createdDate"] = createdDate;
		j["lastModified"] = lastModified;
		return j;
	}

	void ProjectSettings::Deserialize(const nlohmann::json& j) {
		if (j.contains("projectName")) projectName = j["projectName"];
		if (j.contains("projectVersion")) projectVersion = j["projectVersion"];
		if (j.contains("projectDescription")) projectDescription = j["projectDescription"];
		if (j.contains("createdBy")) createdBy = j["createdBy"];
		if (j.contains("createdDate")) createdDate = j["createdDate"];
		if (j.contains("lastModified")) lastModified = j["lastModified"];
	}

	// ProjectManager implementation
	ProjectManager::ProjectManager(GUI::ViewManager& viewMgr, ECS::EntityManager& entityMgr)
		: m_viewManager(viewMgr), m_entityManager(entityMgr) {
		LoadRecentProjects();
	}

	ProjectManager::~ProjectManager() {
		SaveRecentProjects();
	}

	bool ProjectManager::ShouldShowStartup() const {
		// Show startup if no project path is set or no project is open
		return Utils::FilePaths::lastOpenProjectPath.empty() || !m_isProjectOpen;
	}

	bool ProjectManager::CreateNewProject(const std::string& projectPath, const std::string& projectName) {
		try {
			if (projectPath.empty() || projectName.empty()) {
				m_lastError = "Project path and name cannot be empty";
				return false;
			}

			// Close current project if open
			if (m_isProjectOpen) {
				CloseProject();
			}

			// Create project directory
			std::filesystem::create_directories(projectPath);
			std::filesystem::create_directories(projectPath + "/data");
			std::filesystem::create_directories(projectPath + "/assets");
			std::filesystem::create_directories(projectPath + "/output");

			// Initialize project settings
			m_projectSettings = ProjectSettings();
			m_projectSettings.projectName = projectName;
			m_projectSettings.projectVersion = "1.0.0";

			// Set creation date
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			m_projectSettings.createdDate = ss.str();
			m_projectSettings.lastModified = ss.str();

			m_currentProjectPath = std::filesystem::absolute(projectPath).string();
			m_isProjectOpen = true;

			// Reset view state
			m_viewState.Reset();

			// Save project
			if (!SaveProject()) {
				m_lastError = "Failed to save new project";
				return false;
			}

			// Update file paths
			Utils::FilePaths::lastOpenProjectPath = m_currentProjectPath;
			Utils::FilePaths::SaveFilepathDefaults();

			// Add to recent projects
			AddToRecentProjects(m_currentProjectPath);

			std::cout << "[ProjectManager] Created new project: " << projectName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception creating project: " + std::string(e.what());
			return false;
		}
	}

	bool ProjectManager::LoadProject(const std::string& projectPath) {
		try {
			// Check if project file exists
			std::string projectFile = projectPath + "/project.ani";
			if (!std::filesystem::exists(projectFile)) {
				m_lastError = "Project file not found: " + projectFile;
				return false;
			}

			// Close current project if open
			if (m_isProjectOpen) {
				CloseProject();
			}

			// Load project file
			std::ifstream file(projectFile);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file";
				return false;
			}

			nlohmann::json projectData;
			file >> projectData;
			file.close();

			// Deserialize project settings
			if (projectData.contains("settings")) {
				m_projectSettings.Deserialize(projectData["settings"]);
			}

			m_currentProjectPath = std::filesystem::absolute(projectPath).string();
			m_isProjectOpen = true;

			// Load view state
			LoadViewState();

			// Load ImGui layout
			LoadImGuiLayout();

			// Update file paths
			Utils::FilePaths::lastOpenProjectPath = m_currentProjectPath;
			Utils::FilePaths::SaveFilepathDefaults();

			// Add to recent projects
			AddToRecentProjects(m_currentProjectPath);

			std::cout << "[ProjectManager] Loaded project: " << m_projectSettings.projectName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception loading project: " + std::string(e.what());
			return false;
		}
	}

	bool ProjectManager::SaveProject() {
		if (!m_isProjectOpen) {
			m_lastError = "No project is currently open";
			return false;
		}

		try {
			// Update last modified time
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			m_projectSettings.lastModified = ss.str();

			// Create project data
			nlohmann::json projectData;
			projectData["version"] = "1.0";
			projectData["settings"] = m_projectSettings.Serialize();

			// Save project file
			std::string projectFile = m_currentProjectPath + "/project.ani";
			std::ofstream file(projectFile);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file for writing";
				return false;
			}

			file << projectData.dump(4);
			file.close();

			// Save view state
			SaveViewState();

			// Save ImGui layout
			SaveImGuiLayout();

			std::cout << "[ProjectManager] Saved project: " << m_projectSettings.projectName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception saving project: " + std::string(e.what());
			return false;
		}
	}

	void ProjectManager::CloseProject() {
		if (!m_isProjectOpen) return;

		// Save before closing
		SaveProject();

		// Reset state
		m_isProjectOpen = false;
		m_currentProjectPath.clear();
		m_projectSettings = ProjectSettings();
		m_viewState.Reset();

		// Clear file paths
		Utils::FilePaths::lastOpenProjectPath.clear();
		Utils::FilePaths::SaveFilepathDefaults();

		// Show startup view
		m_viewState.SetViewOpen("ProjectManagerView", true);

		std::cout << "[ProjectManager] Project closed" << std::endl;
	}

	std::vector<std::string> ProjectManager::GetRecentProjects() const {
		return m_recentProjects;
	}

	void ProjectManager::AddToRecentProjects(const std::string& projectPath) {
		// Remove if already exists
		auto it = std::find(m_recentProjects.begin(), m_recentProjects.end(), projectPath);
		if (it != m_recentProjects.end()) {
			m_recentProjects.erase(it);
		}

		// Add to front
		m_recentProjects.insert(m_recentProjects.begin(), projectPath);

		// Limit size
		if (m_recentProjects.size() > 10) {
			m_recentProjects.resize(10);
		}

		SaveRecentProjects();
	}

	bool ProjectManager::SaveViewState() {
		if (!m_isProjectOpen) return false;

		try {
			std::string viewStateFile = m_currentProjectPath + "/data/viewstate.json";
			return m_viewState.SaveToFile(viewStateFile);
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to save view state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadViewState() {
		if (!m_isProjectOpen) return false;

		try {
			std::string viewStateFile = m_currentProjectPath + "/data/viewstate.json";
			if (std::filesystem::exists(viewStateFile)) {
				return m_viewState.LoadFromFile(viewStateFile);
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load view state: " << e.what() << std::endl;
		}
		return false;
	}

	bool ProjectManager::SaveImGuiLayout() {
		if (!m_isProjectOpen) return false;

		try {
			// TODO: Implement ImGui layout saving
			// Need to copy current imgui.ini to project data directory
			std::string imguiFile = m_currentProjectPath + "/data/imgui.ini";
			// ImGui::SaveIniSettingsToDisk(imguiFile.c_str());
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to save ImGui layout: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadImGuiLayout() {
		if (!m_isProjectOpen) return false;

		try {
			// TODO: Implement ImGui layout loading
			// Need to load imgui.ini from project data directory
			std::string imguiFile = m_currentProjectPath + "/data/imgui.ini";
			if (std::filesystem::exists(imguiFile)) {
				// ImGui::LoadIniSettingsFromDisk(imguiFile.c_str());
				return true;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load ImGui layout: " << e.what() << std::endl;
		}
		return false;
	}

	void ProjectManager::LoadRecentProjects() {
		try {
			std::string recentFile = "../data/defaults/recent_projects.json";
			if (!std::filesystem::exists(recentFile)) return;

			std::ifstream file(recentFile);
			if (!file.is_open()) return;

			nlohmann::json j;
			file >> j;
			file.close();

			if (j.contains("recentProjects") && j["recentProjects"].is_array()) {
				m_recentProjects.clear();
				for (const auto& path : j["recentProjects"]) {
					if (path.is_string() && std::filesystem::exists(path.get<std::string>())) {
						m_recentProjects.push_back(path.get<std::string>());
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load recent projects: " << e.what() << std::endl;
		}
	}

	void ProjectManager::SaveRecentProjects() {
		try {
			std::filesystem::create_directories("../data/defaults");

			nlohmann::json j;
			j["recentProjects"] = m_recentProjects;

			std::string recentFile = "../data/defaults/recent_projects.json";
			std::ofstream file(recentFile);
			if (file.is_open()) {
				file << j.dump(4);
				file.close();
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to save recent projects: " << e.what() << std::endl;
		}
	}

} // namespace ANI