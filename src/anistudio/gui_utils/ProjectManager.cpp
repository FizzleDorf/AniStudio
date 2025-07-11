#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "FilePaths.hpp"
#include "ImGuiStateUtils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

// Include GLFW for window state management
#include <GLFW/glfw3.h>

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
		// FilePaths utility should already be initialized at application startup
		// We just read the current state
	}

	ProjectManager::~ProjectManager() {
		// Save any changes to FilePaths when the ProjectManager is destroyed
		Utils::FilePaths::SaveFilepathDefaults();
	}

	void ProjectManager::SetWindowHandle(void* windowHandle) {
		m_windowHandle = windowHandle;
		std::cout << "[ProjectManager] Window handle set for window state management" << std::endl;
	}

	void ProjectManager::InitializeApplicationPaths() {
		// This should be called at application startup, before creating ProjectManager
		Utils::FilePaths::Init();
	}

	bool ProjectManager::ShouldShowStartup() const {
		// Use FilePaths utility to check if we should show startup
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

			// Create project directory structure
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

			// Update FilePaths utility with new project info
			Utils::FilePaths::lastOpenProjectPath = m_currentProjectPath;
			UpdateProjectSpecificPaths();

			// Save current window state to the new project
			SaveProjectWindowState();

			// Save project
			if (!SaveProject()) {
				m_lastError = "Failed to save new project";
				return false;
			}

			// Save updated paths to utility
			Utils::FilePaths::SaveFilepathDefaults();

			// Add to recent projects (handled by FilePaths utility)
			AddToRecentProjects(m_currentProjectPath);

			// Call the project created callback
			if (m_onProjectCreatedCallback) {
				m_onProjectCreatedCallback(m_currentProjectPath);
			}

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

			// Update FilePaths utility
			Utils::FilePaths::lastOpenProjectPath = m_currentProjectPath;
			UpdateProjectSpecificPaths();

			// Load project-specific window state and apply it IMMEDIATELY
			LoadAndApplyProjectWindowState();

			// Load view state
			LoadViewState();

			// Load ImGui layout
			LoadImGuiLayout();

			// Save updated paths to utility
			Utils::FilePaths::SaveFilepathDefaults();

			// Add to recent projects
			AddToRecentProjects(m_currentProjectPath);

			// Call the project loaded callback
			if (m_onProjectLoadedCallback) {
				m_onProjectLoadedCallback(m_currentProjectPath);
			}

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

			// Save project window state
			SaveProjectWindowState();

			// Save updated FilePaths
			Utils::FilePaths::SaveFilepathDefaults();

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

		// Save current project's ImGui layout and window state before closing
		SaveImGuiLayout();
		SaveProjectWindowState();

		// Reset state
		m_isProjectOpen = false;
		m_currentProjectPath.clear();
		m_projectSettings = ProjectSettings();
		m_viewState.Reset();

		// Clear project-specific paths in FilePaths utility
		Utils::FilePaths::lastOpenProjectPath.clear();
		ClearProjectSpecificPaths();

		// Save cleared paths
		Utils::FilePaths::SaveFilepathDefaults();

		// Call the project closed callback BEFORE showing startup view
		if (m_onProjectClosedCallback) {
			m_onProjectClosedCallback();
		}

		// Show startup view
		m_viewState.SetViewOpen("ProjectManagerView", true);

		std::cout << "[ProjectManager] Project closed" << std::endl;
	}

	std::vector<std::string> ProjectManager::GetRecentProjects() const {
		// Load recent projects from the JSON file that FilePaths manages
		std::vector<std::string> recentProjects;

		try {
			std::string recentFile = Utils::FilePaths::dataPath + "/recent_projects.json";
			if (!std::filesystem::exists(recentFile)) return recentProjects;

			std::ifstream file(recentFile);
			if (!file.is_open()) return recentProjects;

			nlohmann::json j;
			file >> j;
			file.close();

			if (j.contains("recentProjects") && j["recentProjects"].is_array()) {
				for (const auto& path : j["recentProjects"]) {
					if (path.is_string() && std::filesystem::exists(path.get<std::string>())) {
						recentProjects.push_back(path.get<std::string>());
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load recent projects: " << e.what() << std::endl;
		}

		return recentProjects;
	}

	void ProjectManager::AddToRecentProjects(const std::string& projectPath) {
		try {
			auto recentProjects = GetRecentProjects();

			// Remove if already exists
			auto it = std::find(recentProjects.begin(), recentProjects.end(), projectPath);
			if (it != recentProjects.end()) {
				recentProjects.erase(it);
			}

			// Add to front
			recentProjects.insert(recentProjects.begin(), projectPath);

			// Limit size
			if (recentProjects.size() > 10) {
				recentProjects.resize(10);
			}

			// Save back to file
			std::filesystem::create_directories(Utils::FilePaths::dataPath);

			nlohmann::json j;
			j["recentProjects"] = recentProjects;

			std::string recentFile = Utils::FilePaths::dataPath + "/recent_projects.json";
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

	void ProjectManager::SetDefaultProjectPath(const std::string& path) {
		Utils::FilePaths::defaultProjectPath = path;
		Utils::FilePaths::SaveFilepathDefaults();
	}

	std::string ProjectManager::GetDefaultProjectPath() const {
		return Utils::FilePaths::defaultProjectPath;
	}

	void ProjectManager::SetAssetsFolder(const std::string& path) {
		Utils::FilePaths::assetsFolderPath = path;
		Utils::FilePaths::SaveFilepathDefaults();
	}

	std::string ProjectManager::GetAssetsFolder() const {
		return Utils::FilePaths::assetsFolderPath;
	}

	void ProjectManager::SetOutputFolder(const std::string& path) {
		Utils::FilePaths::outputFolderPath = path;
		Utils::FilePaths::SaveFilepathDefaults();
		std::cout << "[ProjectManager] Set output folder to: " << path << std::endl;
	}

	std::string ProjectManager::GetOutputFolder() const {
		return Utils::FilePaths::outputFolderPath;
	}

	bool ProjectManager::SaveViewState() {
		if (!m_isProjectOpen) return false;

		try {
			std::string viewStateFile = GetProjectDataPath() + "/viewstate.json";
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
			std::string viewStateFile = GetProjectDataPath() + "/viewstate.json";
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
			Utils::ImGuiStateUtils::SaveProjectImGuiLayout(m_currentProjectPath);
			std::cout << "[ProjectManager] Saved ImGui layout for project" << std::endl;
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
			Utils::ImGuiStateUtils::LoadProjectImGuiLayout(m_currentProjectPath);
			std::cout << "[ProjectManager] Loaded ImGui layout for project" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load ImGui layout: " << e.what() << std::endl;
		}
		return false;
	}

	bool ProjectManager::SaveProjectWindowState() {
		if (!m_isProjectOpen || !m_windowHandle) return false;

		try {
			GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_windowHandle);

			// Get current window state
			int width, height, x, y;
			glfwGetWindowSize(glfwWindow, &width, &height);
			glfwGetWindowPos(glfwWindow, &x, &y);
			bool maximized = (glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
			bool fullscreen = (glfwGetWindowMonitor(glfwWindow) != nullptr);

			// Create window state JSON
			nlohmann::json windowState;
			windowState["width"] = width;
			windowState["height"] = height;
			windowState["posX"] = x;
			windowState["posY"] = y;
			windowState["maximized"] = maximized;
			windowState["fullscreen"] = fullscreen;
			windowState["vsync"] = true;
			windowState["title"] = "AniStudio";

			// Save to project
			std::string windowStatePath = GetProjectWindowStatePath();
			std::filesystem::create_directories(std::filesystem::path(windowStatePath).parent_path());

			std::ofstream file(windowStatePath);
			if (file.is_open()) {
				file << windowState.dump(4);
				file.close();
				std::cout << "[ProjectManager] Saved window state for project: "
					<< width << "x" << height << " at (" << x << "," << y << ")" << std::endl;
				return true;
			}
			else {
				std::cerr << "[ProjectManager] Failed to open window state file for writing" << std::endl;
				return false;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to save project window state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadAndApplyProjectWindowState() {
		if (!m_isProjectOpen || !m_windowHandle) return false;

		try {
			std::string windowStatePath = GetProjectWindowStatePath();
			if (std::filesystem::exists(windowStatePath)) {
				// Load the window state JSON
				std::ifstream file(windowStatePath);
				if (file.is_open()) {
					nlohmann::json windowState;
					file >> windowState;
					file.close();

					// Apply the state to the actual GLFW window
					GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_windowHandle);

					int width = windowState.value("width", 1200);
					int height = windowState.value("height", 720);
					int x = windowState.value("posX", 100);
					int y = windowState.value("posY", 100);
					bool maximized = windowState.value("maximized", false);

					// Apply size and position
					glfwSetWindowSize(glfwWindow, width, height);
					glfwSetWindowPos(glfwWindow, x, y);

					// Apply maximized state
					if (maximized) {
						glfwMaximizeWindow(glfwWindow);
					}
					else {
						glfwRestoreWindow(glfwWindow);
					}

					std::cout << "[ProjectManager] Loaded and applied project window state: "
						<< width << "x" << height << " at (" << x << "," << y << ")" << std::endl;

					return true;
				}
				else {
					std::cerr << "[ProjectManager] Failed to open window state file" << std::endl;
					return false;
				}
			}
			else {
				std::cout << "[ProjectManager] No project window state found, using current window" << std::endl;
				return false;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Failed to load project window state: " << e.what() << std::endl;
			return false;
		}
	}

	void ProjectManager::UpdateProjectSpecificPaths() {
		if (!m_isProjectOpen) return;

		// Update project-specific asset folder if not already set
		if (Utils::FilePaths::assetsFolderPath.empty()) {
			Utils::FilePaths::assetsFolderPath = GetProjectAssetsPath();
			std::cout << "[ProjectManager] Set assets folder to: " << Utils::FilePaths::assetsFolderPath << std::endl;
		}

		// Update project-specific output folder 
		Utils::FilePaths::outputFolderPath = GetProjectOutputPath();
		std::cout << "[ProjectManager] Set output folder to: " << Utils::FilePaths::outputFolderPath << std::endl;
	}

	void ProjectManager::ClearProjectSpecificPaths() {
		// Clear any project-specific paths but keep global ones
		// Only clear assetsFolderPath if it was pointing to the closed project
		if (!Utils::FilePaths::assetsFolderPath.empty() &&
			Utils::FilePaths::assetsFolderPath.find(m_currentProjectPath) != std::string::npos) {
			Utils::FilePaths::assetsFolderPath.clear();
			std::cout << "[ProjectManager] Cleared assets folder path" << std::endl;
		}

		// Clear output folder path if it was pointing to the closed project
		if (!Utils::FilePaths::outputFolderPath.empty() &&
			Utils::FilePaths::outputFolderPath.find(m_currentProjectPath) != std::string::npos) {
			Utils::FilePaths::outputFolderPath.clear();
			std::cout << "[ProjectManager] Cleared output folder path" << std::endl;
		}
	}

	std::string ProjectManager::GetProjectDataPath() const {
		return m_currentProjectPath + "/data";
	}

	std::string ProjectManager::GetProjectAssetsPath() const {
		return m_currentProjectPath + "/assets";
	}

	std::string ProjectManager::GetProjectOutputPath() const {
		return m_currentProjectPath + "/output";
	}

	std::string ProjectManager::GetProjectWindowStatePath() const {
		return GetProjectDataPath() + "/window_state.json";
	}

} // namespace ANI