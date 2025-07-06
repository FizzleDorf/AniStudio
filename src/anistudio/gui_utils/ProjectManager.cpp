#include "ProjectManager.hpp"
#include <iostream>
#include <fstream>
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
		j["projectModelRoot"] = projectModelRoot;
		j["projectCheckpointDir"] = projectCheckpointDir;
		j["projectVaeDir"] = projectVaeDir;
		j["projectLoraDir"] = projectLoraDir;
		j["projectControlnetDir"] = projectControlnetDir;
		j["projectUpscaleDir"] = projectUpscaleDir;
		return j;
	}

	void ProjectSettings::Deserialize(const nlohmann::json& j) {
		if (j.contains("projectName")) projectName = j["projectName"];
		if (j.contains("projectVersion")) projectVersion = j["projectVersion"];
		if (j.contains("projectDescription")) projectDescription = j["projectDescription"];
		if (j.contains("createdBy")) createdBy = j["createdBy"];
		if (j.contains("createdDate")) createdDate = j["createdDate"];
		if (j.contains("lastModified")) lastModified = j["lastModified"];
		if (j.contains("projectModelRoot")) projectModelRoot = j["projectModelRoot"];
		if (j.contains("projectCheckpointDir")) projectCheckpointDir = j["projectCheckpointDir"];
		if (j.contains("projectVaeDir")) projectVaeDir = j["projectVaeDir"];
		if (j.contains("projectLoraDir")) projectLoraDir = j["projectLoraDir"];
		if (j.contains("projectControlnetDir")) projectControlnetDir = j["projectControlnetDir"];
		if (j.contains("projectUpscaleDir")) projectUpscaleDir = j["projectUpscaleDir"];
	}

	// ProjectManager implementation
	ProjectManager::ProjectManager(GUI::ViewManager& viewMgr, ECS::EntityManager& entityMgr)
		: m_viewManager(viewMgr), m_entityManager(entityMgr) {
		LoadRecentProjects();
	}

	ProjectManager::~ProjectManager() {
		if (m_isProjectOpen && m_hasUnsavedChanges) {
			std::cout << "[ProjectManager] Warning: Closing with unsaved changes!" << std::endl;
		}
		SaveRecentProjects();
	}

	bool ProjectManager::CreateNewProject(const std::string& projectPath, const std::string& projectName) {
		try {
			// Validate inputs
			if (projectPath.empty() || projectName.empty()) {
				m_lastError = "Project path and name cannot be empty";
				return false;
			}

			if (!ValidateProjectPath(projectPath)) {
				m_lastError = "Invalid project path: " + projectPath;
				return false;
			}

			// Close current project if open
			if (m_isProjectOpen) {
				CloseProject();
			}

			// Create project directory structure
			if (!CreateProjectDirectory(projectPath)) {
				return false;
			}

			if (!SetupProjectStructure(projectPath)) {
				return false;
			}

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

			// Set project paths
			m_currentProjectPath = std::filesystem::absolute(projectPath).string();

			// Initialize project-specific model paths
			std::filesystem::path projPath(m_currentProjectPath);
			m_projectSettings.projectModelRoot = (projPath / "models").string();
			m_projectSettings.projectCheckpointDir = (projPath / "models" / "checkpoints").string();
			m_projectSettings.projectVaeDir = (projPath / "models" / "vae").string();
			m_projectSettings.projectLoraDir = (projPath / "models" / "loras").string();
			m_projectSettings.projectControlnetDir = (projPath / "models" / "controlnet").string();
			m_projectSettings.projectUpscaleDir = (projPath / "models" / "upscale_models").string();

			// Update global file paths
			UpdateFilePaths();

			// Reset view state to default
			m_viewState.Reset();
			m_viewState.CreateDefaultWorkspace();

			// Mark as open and save
			m_isProjectOpen = true;
			m_hasUnsavedChanges = true;

			if (!SaveProject()) {
				m_lastError = "Failed to save new project";
				return false;
			}

			// Add to recent projects
			AddToRecentProjects(m_currentProjectPath);

			std::cout << "[ProjectManager] Created new project: " << projectName << " at " << m_currentProjectPath << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception creating project: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadProject(const std::string& projectPath) {
		try {
			// Validate project file exists
			std::filesystem::path projFile = std::filesystem::path(projectPath) / "project.ani";
			if (!std::filesystem::exists(projFile)) {
				m_lastError = "Project file not found: " + projFile.string();
				return false;
			}

			// Close current project if open
			if (m_isProjectOpen) {
				CloseProject();
			}

			// Load project file
			std::ifstream file(projFile);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file: " + projFile.string();
				return false;
			}

			nlohmann::json projectData;
			file >> projectData;
			file.close();

			// Deserialize project data
			if (!DeserializeProjectData(projectData)) {
				return false;
			}

			m_currentProjectPath = std::filesystem::absolute(projectPath).string();

			// Update file paths
			UpdateFilePaths();

			// Load view state if it exists
			LoadViewState();

			m_isProjectOpen = true;
			m_hasUnsavedChanges = false;

			// Add to recent projects
			AddToRecentProjects(m_currentProjectPath);

			std::cout << "[ProjectManager] Loaded project: " << m_projectSettings.projectName << " from " << m_currentProjectPath << std::endl;
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

		try {
			// Update last modified time
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
			m_projectSettings.lastModified = ss.str();

			// Serialize project data
			nlohmann::json projectData = SerializeProjectData();

			// Save to project file
			std::string projectFilePath = GetProjectFilePath();
			std::ofstream file(projectFilePath);
			if (!file.is_open()) {
				m_lastError = "Failed to open project file for writing: " + projectFilePath;
				return false;
			}

			file << projectData.dump(4);
			file.close();

			// Save view state
			SaveViewState();

			m_hasUnsavedChanges = false;

			std::cout << "[ProjectManager] Saved project: " << m_projectSettings.projectName << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Exception saving project: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	bool ProjectManager::SaveProjectAs(const std::string& newPath) {
		std::string oldPath = m_currentProjectPath;
		m_currentProjectPath = std::filesystem::absolute(newPath).string();

		// Create new directory structure
		if (!CreateProjectDirectory(newPath) || !SetupProjectStructure(newPath)) {
			m_currentProjectPath = oldPath; // Restore old path
			return false;
		}

		// Update paths and save
		UpdateFilePaths();
		bool success = SaveProject();

		if (success) {
			AddToRecentProjects(m_currentProjectPath);
		}
		else {
			m_currentProjectPath = oldPath; // Restore old path on failure
		}

		return success;
	}

	void ProjectManager::CloseProject() {
		if (!m_isProjectOpen) return;

		if (m_hasUnsavedChanges) {
			std::cout << "[ProjectManager] Warning: Closing project with unsaved changes!" << std::endl;
			// In a real implementation, you'd show a dialog here
		}

		// Save current view state before closing
		SaveViewState();

		// Reset state
		m_isProjectOpen = false;
		m_hasUnsavedChanges = false;
		m_currentProjectPath.clear();
		m_projectSettings = ProjectSettings();
		m_viewState.Reset();

		// Reset global file paths to defaults
		Utils::FilePaths::LoadFilePathDefaults();

		std::cout << "[ProjectManager] Project closed" << std::endl;
	}

	bool ProjectManager::SaveViewState() {
		if (!m_isProjectOpen) return false;

		try {
			// Capture current view state from ViewManager
			m_viewState.CaptureCurrentState(m_viewManager);

			// Save to file
			std::string viewStateFile = GetViewStateFilePath();
			return m_viewState.SaveToFile(viewStateFile);
		}
		catch (const std::exception& e) {
			m_lastError = "Failed to save view state: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			return false;
		}
	}

	bool ProjectManager::LoadViewState() {
		if (!m_isProjectOpen) return false;

		try {
			std::string viewStateFile = GetViewStateFilePath();
			if (!std::filesystem::exists(viewStateFile)) {
				// No saved view state, create default
				m_viewState.CreateDefaultWorkspace();
				return true;
			}

			// Load from file
			if (!m_viewState.LoadFromFile(viewStateFile)) {
				std::cerr << "[ProjectManager] Failed to load view state, using defaults" << std::endl;
				m_viewState.CreateDefaultWorkspace();
				return false;
			}

			// Restore state to ViewManager
			return m_viewState.RestoreState(m_viewManager, m_entityManager);
		}
		catch (const std::exception& e) {
			m_lastError = "Failed to load view state: " + std::string(e.what());
			std::cerr << "[ProjectManager] " << m_lastError << std::endl;
			m_viewState.CreateDefaultWorkspace();
			return false;
		}
	}

	bool ProjectManager::HasSavedViewState() const {
		if (!m_isProjectOpen) return false;
		return std::filesystem::exists(GetViewStateFilePath());
	}

	void ProjectManager::Update(float deltaTime) {
		if (!m_isProjectOpen || !m_autoSaveEnabled) return;

		m_autoSaveTimer += deltaTime;
		if (m_autoSaveTimer >= m_autoSaveInterval && m_hasUnsavedChanges) {
			std::cout << "[ProjectManager] Auto-saving project..." << std::endl;
			SaveProject();
			m_autoSaveTimer = 0.0f;
		}
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
		if (m_recentProjects.size() > MAX_RECENT_PROJECTS) {
			m_recentProjects.resize(MAX_RECENT_PROJECTS);
		}

		SaveRecentProjects();
	}

	// Private helper methods
	std::string ProjectManager::GetProjectFilePath() const {
		return (std::filesystem::path(m_currentProjectPath) / "project.ani").string();
	}

	std::string ProjectManager::GetViewStateFilePath() const {
		return (std::filesystem::path(m_currentProjectPath) / "viewstate.json").string();
	}

	std::string ProjectManager::GetSettingsFilePath() const {
		return (std::filesystem::path(m_currentProjectPath) / "settings.json").string();
	}

	std::string ProjectManager::GetAssetsDirectoryPath() const {
		return (std::filesystem::path(m_currentProjectPath) / "assets").string();
	}

	std::string ProjectManager::GetScriptsDirectoryPath() const {
		return (std::filesystem::path(m_currentProjectPath) / "scripts").string();
	}

	std::string ProjectManager::GetModelsDirectoryPath() const {
		return (std::filesystem::path(m_currentProjectPath) / "models").string();
	}

	bool ProjectManager::CreateProjectDirectory(const std::string& projectPath) {
		try {
			std::filesystem::create_directories(projectPath);
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Failed to create project directory: " + std::string(e.what());
			return false;
		}
	}

	bool ProjectManager::SetupProjectStructure(const std::string& projectPath) {
		try {
			std::filesystem::path projPath(projectPath);

			// Create standard project directories
			std::filesystem::create_directories(projPath / "assets");
			std::filesystem::create_directories(projPath / "scripts");
			std::filesystem::create_directories(projPath / "output");
			std::filesystem::create_directories(projPath / "temp");
			std::filesystem::create_directories(projPath / "models");
			std::filesystem::create_directories(projPath / "models" / "checkpoints");
			std::filesystem::create_directories(projPath / "models" / "vae");
			std::filesystem::create_directories(projPath / "models" / "loras");
			std::filesystem::create_directories(projPath / "models" / "controlnet");
			std::filesystem::create_directories(projPath / "models" / "upscale_models");
			std::filesystem::create_directories(projPath / "data");

			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Failed to setup project structure: " + std::string(e.what());
			return false;
		}
	}

	void ProjectManager::UpdateFilePaths() {
		if (!m_isProjectOpen) return;

		// Update global file paths with project-specific paths
		Utils::FilePaths::lastOpenProjectPath = m_currentProjectPath;
		Utils::FilePaths::defaultProjectPath = m_currentProjectPath;
		Utils::FilePaths::assetsFolderPath = GetAssetsDirectoryPath();
		Utils::FilePaths::defaultScriptsPath = GetScriptsDirectoryPath();

		// Update model paths
		Utils::FilePaths::defaultModelRootPath = m_projectSettings.projectModelRoot;
		Utils::FilePaths::checkpointDir = m_projectSettings.projectCheckpointDir;
		Utils::FilePaths::vaeDir = m_projectSettings.projectVaeDir;
		Utils::FilePaths::loraDir = m_projectSettings.projectLoraDir;
		Utils::FilePaths::controlnetDir = m_projectSettings.projectControlnetDir;
		Utils::FilePaths::upscaleDir = m_projectSettings.projectUpscaleDir;

		// Save updated paths
		Utils::FilePaths::SaveFilepathDefaults();

		MarkAsModified();
	}

	void ProjectManager::MarkAsModified() {
		m_hasUnsavedChanges = true;
		m_autoSaveTimer = 0.0f; // Reset auto-save timer
	}

	bool ProjectManager::ValidateProjectPath(const std::string& path) const {
		if (path.empty()) return false;

		try {
			std::filesystem::path fsPath(path);
			// Check if path is valid and can be created
			return !fsPath.empty();
		}
		catch (const std::exception&) {
			return false;
		}
	}

	nlohmann::json ProjectManager::SerializeProjectData() const {
		nlohmann::json projectData;
		projectData["version"] = "1.0";
		projectData["settings"] = m_projectSettings.Serialize();
		return projectData;
	}

	bool ProjectManager::DeserializeProjectData(const nlohmann::json& j) {
		try {
			if (!j.contains("settings")) {
				m_lastError = "Invalid project file: missing settings";
				return false;
			}

			m_projectSettings.Deserialize(j["settings"]);
			return true;
		}
		catch (const std::exception& e) {
			m_lastError = "Failed to deserialize project data: " + std::string(e.what());
			return false;
		}
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

	bool ProjectManager::CreateProjectFromTemplate(const std::string& templateName, const std::string& projectPath, const std::string& projectName) {
		// For now, just create a standard project
		// In the future, have different templates with different setups
		return CreateNewProject(projectPath, projectName);
	}

	std::vector<std::string> ProjectManager::GetAvailableTemplates() const {
		// Return available templates
		return { "Default", "Animation", "Still Image", "Video Processing" };
	}

} // namespace ANI