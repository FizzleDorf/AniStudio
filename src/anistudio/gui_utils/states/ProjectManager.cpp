#include "ProjectManager.hpp"
#include "ViewManager.hpp"
#include "WindowState.hpp"
#include "ImGuiStateUtils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>

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
		std::cout << "[ProjectManager] Initialized" << std::endl;

		// Ensure FilePaths is initialized
		if (!Utils::FilePaths::initialized) {
			std::cout << "[ProjectManager] FilePaths not initialized, initializing now..." << std::endl;
			Utils::FilePaths::Init();
		}

		// Debug current FilePaths state
		std::cout << "[ProjectManager] Current FilePaths state:" << std::endl;
		std::cout << "  - Default project path: '" << Utils::FilePaths::defaultProjectPath << "'" << std::endl;
		std::cout << "  - Last opened project: '" << Utils::FilePaths::lastOpenProjectPath << "'" << std::endl;
		std::cout << "  - Data path: '" << Utils::FilePaths::dataPath << "'" << std::endl;
	}

	ProjectManager::~ProjectManager() {}

	void ProjectManager::SetWindowHandle(void* windowHandle) {
		m_windowHandle = windowHandle;
		std::cout << "[ProjectManager] Window handle set for project window state management" << std::endl;
	}

	bool ProjectManager::ShouldShowStartup() const {
		// Check if we have a last opened project
		bool hasLastProject = !Utils::FilePaths::lastOpenProjectPath.empty();

		// Debug output
		std::cout << "[ProjectManager] ShouldShowStartup check:" << std::endl;
		std::cout << "  - Last opened project: '" << Utils::FilePaths::lastOpenProjectPath << "'" << std::endl;
		std::cout << "  - Has last project: " << (hasLastProject ? "YES" : "NO") << std::endl;
		std::cout << "  - Default project path: '" << Utils::FilePaths::defaultProjectPath << "'" << std::endl;

		// Try to find any existing projects
		auto recentProjects = GetRecentProjects();
		std::cout << "  - Found " << recentProjects.size() << " recent projects" << std::endl;
		for (const auto& proj : recentProjects) {
			std::cout << "    * " << proj << std::endl;
		}

		// Show startup if no recent projects found
		bool shouldShow = recentProjects.empty();
		std::cout << "  - Should show startup: " << (shouldShow ? "YES" : "NO") << std::endl;

		return shouldShow;
	}

	bool ProjectManager::CreateNewProject(const std::string& projectPath, const std::string& projectName) {
		m_lastError.clear();

		try {
			// Validate project path
			std::filesystem::path projPath(projectPath);
			if (std::filesystem::exists(projPath)) {
				m_lastError = "Project directory already exists: " + projectPath;
				return false;
			}

			// Create project directory
			std::filesystem::create_directories(projPath);

			// Create subdirectories
			std::filesystem::create_directories(projPath / "data");
			std::filesystem::create_directories(projPath / "assets");
			std::filesystem::create_directories(projPath / "output");
			std::filesystem::create_directories(projPath / "settings");

			// Setup project settings
			m_projectSettings = ProjectSettings{};
			m_projectSettings.projectName = projectName;
			m_projectSettings.projectVersion = "1.0.0";

			// Get current date/time
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			m_projectSettings.createdDate = ss.str();
			m_projectSettings.lastModified = ss.str();

			// Set current project path
			m_currentProjectPath = projectPath;
			m_isProjectOpen = true;

			// Reset ViewState and create default workspace
			m_viewState.Reset();

			// CRITICAL: ALWAYS CREATE A DEFAULT WORKSPACE WHEN CREATING NEW PROJECT
			GUI::WorkspaceID defaultWorkspace = m_viewManager.CreateView();
			m_viewState.SetLastActiveWorkspace(defaultWorkspace);
			m_viewManager.SetActiveWorkspace(defaultWorkspace);
			std::cout << "[ProjectManager] Created default workspace: " << defaultWorkspace << std::endl;

			// Save project files
			if (!SaveProject()) {
				m_lastError = "Failed to save new project";
				m_isProjectOpen = false;
				m_currentProjectPath.clear();
				return false;
			}

			// Update project-specific paths
			UpdateProjectSpecificPaths();

			// Add to recent projects
			AddToRecentProjects(projectPath);

			std::cout << "[ProjectManager] Created new project: " << projectName << " at " << projectPath << std::endl;

			// Trigger callback
			if (m_onProjectCreatedCallback) {
				m_onProjectCreatedCallback(projectPath);
			}

			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception creating project: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadProject(const std::string& projectPath) {
		m_lastError.clear();

		try {
			// Validate project path
			std::filesystem::path projPath(projectPath);
			if (!std::filesystem::exists(projPath)) {
				m_lastError = "Project directory does not exist: " + projectPath;
				return false;
			}

			// Check for project file
			std::filesystem::path projectFile = projPath / "project.ani";
			if (!std::filesystem::exists(projectFile)) {
				m_lastError = "Project file not found: " + projectFile.string();
				return false;
			}

			// Close existing project if open
			if (m_isProjectOpen) {
				CloseProject();
			}

			// Load project settings
			std::ifstream file(projectFile);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file: " + projectFile.string();
				return false;
			}

			nlohmann::json projectJson;
			file >> projectJson;
			file.close();

			// Extract settings from the nested structure
			if (projectJson.contains("settings")) {
				m_projectSettings.Deserialize(projectJson["settings"]);
			}
			else {
				m_lastError = "Invalid project file format: missing settings";
				return false;
			}

			// Set current project path
			m_currentProjectPath = projectPath;
			m_isProjectOpen = true;

			// Update project-specific paths FIRST
			UpdateProjectSpecificPaths();

			// Add to recent projects
			AddToRecentProjects(projectPath);

			std::cout << "[ProjectManager] Loaded project: " << m_projectSettings.projectName << " from " << projectPath << std::endl;

			// CRITICAL: Trigger callback FIRST - this loads plugins so view types get registered
			if (m_onProjectLoadedCallback) {
				m_onProjectLoadedCallback(projectPath);
			}

			// NOW load ViewState AFTER plugins are loaded and view types are registered
			bool viewStateLoaded = LoadViewState();

			// ALWAYS CREATE A DEFAULT WORKSPACE IF NONE EXIST
			auto allWorkspaces = m_viewManager.GetAllWorkspaces();
			if (allWorkspaces.empty()) {
				std::cout << "[ProjectManager] No workspaces found, creating default workspace" << std::endl;
				GUI::WorkspaceID defaultWorkspace = m_viewManager.CreateView();
				m_viewState.SetLastActiveWorkspace(defaultWorkspace);
				m_viewManager.SetActiveWorkspace(defaultWorkspace);
				std::cout << "[ProjectManager] Created default workspace: " << defaultWorkspace << std::endl;
			}
			else {
				std::cout << "[ProjectManager] Loaded " << allWorkspaces.size() << " workspaces from project" << std::endl;
			}

			// Load ImGui layout (if exists)
			LoadImGuiLayout();

			// Load and apply project window state
			LoadAndApplyProjectWindowState();

			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception loading project: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	bool ProjectManager::SaveProject() {
		if (!m_isProjectOpen) {
			m_lastError = "No project is currently open";
			return false;
		}

		m_lastError.clear();

		try {
			// Update last modified time
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			m_projectSettings.lastModified = ss.str();

			// Save project settings in .ani format
			std::filesystem::path projectFile = std::filesystem::path(m_currentProjectPath) / "project.ani";
			std::ofstream file(projectFile);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file for writing: " + projectFile.string();
				return false;
			}

			// Create the project structure with nested settings
			nlohmann::json projectJson;
			projectJson["version"] = "1.0";
			projectJson["settings"] = m_projectSettings.Serialize();

			file << projectJson.dump(4);
			file.close();

			// CRITICAL: Save ViewState which includes active workspace and all workspace configurations
			if (!SaveViewState()) {
				std::cout << "[ProjectManager] Warning: Failed to save ViewState" << std::endl;
			}

			// Save ImGui layout
			if (!SaveImGuiLayout()) {
				std::cout << "[ProjectManager] Warning: Failed to save ImGui layout" << std::endl;
			}

			// Save window state
			if (!SaveProjectWindowState()) {
				std::cout << "[ProjectManager] Warning: Failed to save window state" << std::endl;
			}

			std::cout << "[ProjectManager] Project saved: " << m_projectSettings.projectName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception saving project: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	void ProjectManager::CloseProject() {
		if (!m_isProjectOpen) {
			std::cout << "[ProjectManager] No project to close" << std::endl;
			return;
		}

		std::cout << "[ProjectManager] CloseProject() called - project open: " << m_isProjectOpen << std::endl;
		std::cout << "[ProjectManager] Closing project: " << m_projectSettings.projectName << std::endl;

		// Save project before closing (this will save ViewState, ImGui layout, etc.)
		std::cout << "[ProjectManager] Saving project before closing..." << std::endl;
		SaveProject();

		// Reset the ViewManager to clean state - THIS CLEARS ALL WORKSPACES
		std::cout << "[ProjectManager] Resetting ViewManager..." << std::endl;
		m_viewManager.Reset(); // Soft reset - keeps registered views BUT CLEARS ALL WORKSPACES

		// Clear project-specific paths
		ClearProjectSpecificPaths();

		// Reset state
		std::cout << "[ProjectManager] Clearing project state..." << std::endl;
		m_isProjectOpen = false;
		m_currentProjectPath.clear();
		m_projectSettings = ProjectSettings{};
		m_viewState.Reset();

		std::cout << "[ProjectManager] Project closed and ALL workspaces cleared" << std::endl;

		// CRITICAL: Trigger callback
		std::cout << "[ProjectManager] Triggering OnProjectClosed callback..." << std::endl;
		if (m_onProjectClosedCallback) {
			m_onProjectClosedCallback();
			std::cout << "[ProjectManager] OnProjectClosed callback completed" << std::endl;
		}
		else {
			std::cout << "[ProjectManager] ERROR: No OnProjectClosed callback set!" << std::endl;
		}
	}

	bool ProjectManager::IsProjectNameTaken(const std::string& projectName, const std::string& excludePath) const {
		// Get default project path
		std::string defaultPath = GetDefaultProjectPath();
		if (defaultPath.empty()) return false;

		std::filesystem::path projectDir = std::filesystem::path(defaultPath) / projectName;

		// If this is the path we're excluding, it's not "taken"
		if (!excludePath.empty() && std::filesystem::equivalent(projectDir, excludePath)) {
			return false;
		}

		return std::filesystem::exists(projectDir);
	}

	std::vector<std::string> ProjectManager::GetRecentProjects() const {
		std::vector<std::string> recentProjects;
		std::set<std::string> addedPaths; // Track added paths to prevent duplicates

		// First, check if we have a last opened project
		if (!Utils::FilePaths::lastOpenProjectPath.empty() &&
			std::filesystem::exists(Utils::FilePaths::lastOpenProjectPath)) {

			try {
				// Normalize the path to handle different representations of the same path
				std::string normalizedPath = std::filesystem::canonical(Utils::FilePaths::lastOpenProjectPath).string();
				recentProjects.push_back(normalizedPath);
				addedPaths.insert(normalizedPath);
			}
			catch (const std::exception& e) {
				// If canonical fails, use the original path
				recentProjects.push_back(Utils::FilePaths::lastOpenProjectPath);
				addedPaths.insert(Utils::FilePaths::lastOpenProjectPath);
			}
		}

		// Then scan the default project directory for any existing projects
		std::string defaultProjectDir = Utils::FilePaths::defaultProjectPath;
		if (!defaultProjectDir.empty() && std::filesystem::exists(defaultProjectDir)) {
			try {
				for (const auto& entry : std::filesystem::directory_iterator(defaultProjectDir)) {
					if (entry.is_directory()) {
						std::string projectPath = entry.path().string();

						// Check if this directory contains a project.ani file (indicating it's a project)
						std::string projectFile = projectPath + "/project.ani";
						if (std::filesystem::exists(projectFile)) {
							try {
								// Normalize the path to handle different representations
								std::string normalizedPath = std::filesystem::canonical(projectPath).string();

								// Don't add duplicates
								if (addedPaths.find(normalizedPath) == addedPaths.end()) {
									recentProjects.push_back(normalizedPath);
									addedPaths.insert(normalizedPath);
								}
							}
							catch (const std::exception& e) {
								// If canonical fails, check with original path
								if (addedPaths.find(projectPath) == addedPaths.end()) {
									recentProjects.push_back(projectPath);
									addedPaths.insert(projectPath);
								}
							}
						}
					}
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[ProjectManager] Error scanning for projects: " << e.what() << std::endl;
			}
		}

		// Sort by most recently modified
		std::sort(recentProjects.begin(), recentProjects.end(),
			[](const std::string& a, const std::string& b) {
			try {
				auto timeA = std::filesystem::last_write_time(a + "/project.ani");
				auto timeB = std::filesystem::last_write_time(b + "/project.ani");
				return timeA > timeB; // Most recent first
			}
			catch (...) {
				return false;
			}
		});

		return recentProjects;
	}

	void ProjectManager::AddToRecentProjects(const std::string& projectPath) {
		// Update the last opened project path in FilePaths
		Utils::FilePaths::lastOpenProjectPath = projectPath;
		Utils::FilePaths::SaveFilepathDefaults();
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
	}

	std::string ProjectManager::GetOutputFolder() const {
		return Utils::FilePaths::outputFolderPath;
	}

	void ProjectManager::InitializeApplicationPaths() {
		Utils::FilePaths::Init();
	}

	bool ProjectManager::ApplyProjectTemplate(const GUI::ProjectTemplate& template_) {
		if (!m_isProjectOpen) {
			m_lastError = "No project is currently open";
			return false;
		}

		try {
			std::cout << "[ProjectManager] Applying project template: " << template_.name << std::endl;

			// Get the current workspace (should be the default one created during project creation)
			GUI::WorkspaceID currentWorkspace = m_viewState.GetLastActiveWorkspace();

			// Set the workspace name to the template name if provided
			if (!template_.name.empty()) {
				m_viewManager.SetWorkspaceName(currentWorkspace, template_.name);
			}

			// Add the default views from the template
			for (const auto& viewTypeName : template_.defaultOpenViews) {
				try {
					std::cout << "[ProjectManager] Adding view: " << viewTypeName << " to workspace: " << currentWorkspace << std::endl;

					// Get the view type ID from the name
					GUI::ViewTypeID viewType = m_viewManager.GetViewType(viewTypeName);

					// Add the view to the current workspace
					m_viewManager.AddViewByType(currentWorkspace, viewType);

					std::cout << "[ProjectManager] Successfully added view: " << viewTypeName << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[ProjectManager] Failed to add view " << viewTypeName << ": " << e.what() << std::endl;
				}
			}

			// Apply any template settings if provided
			if (!template_.settings.empty()) {
				// TODO: Apply template-specific settings to project
				std::cout << "[ProjectManager] Template has settings (not implemented yet)" << std::endl;
			}

			// Save the project with the new template configuration
			SaveProject();

			std::cout << "[ProjectManager] Successfully applied template: " << template_.name << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception applying project template: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	// Private methods
	bool ProjectManager::SaveViewState() {
		try {
			std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";

			// CRITICAL: Save the current active workspace before serializing
			std::cout << "[ProjectManager] Saving ViewState with active workspace: " << m_viewState.GetLastActiveWorkspace() << std::endl;

			return m_viewState.SaveViewManagerState(m_viewManager, viewStatePath);
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception saving ViewState: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadViewState() {
		try {
			std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";
			bool success = m_viewState.LoadViewManagerState(m_viewManager, viewStatePath);

			if (success) {
				// CRITICAL: After loading ViewState, ensure the active workspace is valid
				GUI::WorkspaceID lastActiveWorkspace = m_viewState.GetLastActiveWorkspace();
				auto allWorkspaces = m_viewManager.GetAllWorkspaces();

				if (std::find(allWorkspaces.begin(), allWorkspaces.end(), lastActiveWorkspace) == allWorkspaces.end()) {
					// Active workspace doesn't exist, use the first available or create one
					if (!allWorkspaces.empty()) {
						lastActiveWorkspace = allWorkspaces[0];
						m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
						std::cout << "[ProjectManager] Corrected active workspace to: " << lastActiveWorkspace << std::endl;
					}
					else {
						// No workspaces exist, create a default one
						lastActiveWorkspace = m_viewManager.CreateView();
						m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
						std::cout << "[ProjectManager] Created default workspace: " << lastActiveWorkspace << std::endl;
					}
				}

				// Set the active workspace in ViewManager
				m_viewManager.SetActiveWorkspace(lastActiveWorkspace);
				std::cout << "[ProjectManager] Loaded ViewState with active workspace: " << lastActiveWorkspace << std::endl;

				// Trigger callback to set the active workspace
				if (m_onViewStateLoadedCallback) {
					m_onViewStateLoadedCallback(lastActiveWorkspace);
				}
			}

			return success;
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception loading ViewState: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::SaveImGuiLayout() {
		try {
			Utils::ImGuiStateUtils::SaveProjectImGuiLayout(m_currentProjectPath);
			return true; // Assume success since the utility function is void
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception saving ImGui layout: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadImGuiLayout() {
		try {
			Utils::ImGuiStateUtils::LoadProjectImGuiLayout(m_currentProjectPath);
			return true; // Assume success since the utility function is void
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception loading ImGui layout: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::SaveProjectWindowState() {
		if (!m_windowHandle) return false;

		try {
			// Create a window state and manually sync from GLFW
			Utils::WindowState windowState;
			windowState.SetGlobalDataPath(GetProjectDataPath());

			// Manually sync current window state using GLFW functions
			GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_windowHandle);
			int width, height, x, y;
			glfwGetWindowSize(glfwWindow, &width, &height);
			glfwGetWindowPos(glfwWindow, &x, &y);

			// Create JSON and deserialize it into WindowState
			nlohmann::json currentState;
			currentState["width"] = width;
			currentState["height"] = height;
			currentState["posX"] = x;
			currentState["posY"] = y;
			currentState["maximized"] = (glfwGetWindowAttrib(glfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
			currentState["fullscreen"] = (glfwGetWindowMonitor(glfwWindow) != nullptr);
			currentState["vsync"] = true;
			currentState["title"] = "AniStudio";

			windowState.Deserialize(currentState);

			// Save to project-specific location
			std::string windowStatePath = GetProjectWindowStatePath();
			return windowState.SaveToFile(windowStatePath);
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception saving window state: " << e.what() << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadAndApplyProjectWindowState() {
		if (!m_windowHandle) return false;

		try {
			std::string windowStatePath = GetProjectWindowStatePath();
			if (!std::filesystem::exists(windowStatePath)) {
				return false; // No saved state, which is fine
			}

			Utils::WindowState windowState;
			windowState.SetGlobalDataPath(GetProjectDataPath());

			if (windowState.LoadFromFile(windowStatePath)) {
				// Manually apply to GLFW window using the same approach as AniStudio.cpp
				GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_windowHandle);
				glfwSetWindowSize(glfwWindow, windowState.GetWidth(), windowState.GetHeight());
				glfwSetWindowPos(glfwWindow, windowState.GetPosX(), windowState.GetPosY());

				if (windowState.IsMaximized()) {
					glfwMaximizeWindow(glfwWindow);
				}
				else {
					glfwRestoreWindow(glfwWindow);
				}

				std::cout << "[ProjectManager] Applied project window state" << std::endl;
				return true;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectManager] Exception loading window state: " << e.what() << std::endl;
		}

		return false;
	}

	void ProjectManager::UpdateProjectSpecificPaths() {
		// For now, just set the paths directly in the FilePaths static variables
		// In a future update, we could implement project-specific path management
		if (!GetProjectAssetsPath().empty()) {
			Utils::FilePaths::assetsFolderPath = GetProjectAssetsPath();
		}
		if (!GetProjectOutputPath().empty()) {
			Utils::FilePaths::outputFolderPath = GetProjectOutputPath();
		}
		Utils::FilePaths::SaveFilepathDefaults();
	}

	void ProjectManager::ClearProjectSpecificPaths() {
		// Clear project-specific paths back to defaults
		// This could be enhanced to restore previous global defaults
		Utils::FilePaths::assetsFolderPath = "";
		Utils::FilePaths::outputFolderPath = "";
		Utils::FilePaths::SaveFilepathDefaults();
	}

	std::string ProjectManager::GetProjectDataPath() const {
		if (!m_isProjectOpen) return "";
		return m_currentProjectPath + "/data";
	}

	std::string ProjectManager::GetProjectAssetsPath() const {
		if (!m_isProjectOpen) return "";
		return m_currentProjectPath + "/assets";
	}

	std::string ProjectManager::GetProjectOutputPath() const {
		if (!m_isProjectOpen) return "";
		return m_currentProjectPath + "/output";
	}

	std::string ProjectManager::GetProjectWindowStatePath() const {
		if (!m_isProjectOpen) return "";
		return GetProjectDataPath() + "/window_state.json";
	}

} // namespace ANI