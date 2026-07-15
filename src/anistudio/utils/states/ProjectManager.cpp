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

    ProjectManager::ProjectManager(GUI::ViewManager& viewMgr, ECS::EntityManager& entityMgr)
        : m_viewManager(viewMgr), m_entityManager(entityMgr) {
        std::cout << "[ProjectManager] Initialized" << std::endl;
    }

    ProjectManager::~ProjectManager() {}

    std::shared_ptr<ECS::FilePathSystem> ProjectManager::GetFilePathSystem() const {
        return m_entityManager.GetSystem<ECS::FilePathSystem>();
    }

    void ProjectManager::SetWindowHandle(void* windowHandle) {
        m_windowHandle = windowHandle;
        std::cout << "[ProjectManager] Window handle set" << std::endl;
    }

    bool ProjectManager::ShouldShowStartup() const {
        std::cout << "[ProjectManager] ShouldShowStartup check:" << std::endl;
        std::cout << "  - Project open: " << (m_isProjectOpen ? "YES" : "NO") << std::endl;
        if (m_isProjectOpen) {
            std::cout << "  - Project already open" << std::endl;
            return false;
        }

        auto fileSys = GetFilePathSystem();
        std::string lastProjectPath = fileSys ? fileSys->GetPath("LastOpenProject") : "";
        if (!lastProjectPath.empty() && std::filesystem::exists(lastProjectPath)) {
            std::cout << "  - Has last opened project: " << lastProjectPath << std::endl;
            const_cast<ProjectManager*>(this)->LoadProject(lastProjectPath);
            return false;
        }

        std::cout << "  - No last opened project" << std::endl;
        std::cout << "  - Should show startup: YES" << std::endl;
        return true;
    }

    bool ProjectManager::CreateNewProject(const std::string& projectPath, const std::string& projectName) {
        m_lastError.clear();
        try {
            std::filesystem::path projPath(projectPath);
            if (std::filesystem::exists(projPath)) {
                m_lastError = "Project directory already exists: " + projectPath;
                return false;
            }

            std::filesystem::create_directories(projPath);
            std::filesystem::create_directories(projPath / "data");
            std::filesystem::create_directories(projPath / "assets");
            std::filesystem::create_directories(projPath / "output");
            std::filesystem::create_directories(projPath / "settings");

            m_projectSettings = ProjectSettings{};
            m_projectSettings.projectName = projectName;
            m_projectSettings.projectVersion = "1.0.0";

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            m_projectSettings.createdDate = ss.str();
            m_projectSettings.lastModified = ss.str();

            m_currentProjectPath = projectPath;
            m_isProjectOpen = true;

            UpdateProjectSpecificPaths();

            auto fileSys = GetFilePathSystem();
            if (fileSys) {
                fileSys->SetPath("LastOpenProject", projectPath);
            }

            m_viewState.Reset();
            GUI::WorkspaceID defaultWorkspace = m_viewManager.CreateView();
            m_viewState.SetLastActiveWorkspace(defaultWorkspace);
            m_viewManager.SetActiveWorkspace(defaultWorkspace);
            std::cout << "[ProjectManager] Created default workspace: " << defaultWorkspace << std::endl;

            if (!SaveProject()) {
                m_lastError = "Failed to save new project";
                m_isProjectOpen = false;
                m_currentProjectPath.clear();
                return false;
            }

            AddToRecentProjects(projectPath);

            std::cout << "[ProjectManager] Created new project: " << projectName << " at " << projectPath << std::endl;
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
            std::filesystem::path projPath(projectPath);
            if (!std::filesystem::exists(projPath)) {
                m_lastError = "Project directory does not exist: " + projectPath;
                return false;
            }

            std::filesystem::path projectFile = projPath / "project.ani";
            if (!std::filesystem::exists(projectFile)) {
                m_lastError = "Project file not found: " + projectFile.string();
                return false;
            }

            if (m_isProjectOpen) {
                CloseProject();
            }

            std::ifstream file(projectFile);
            if (!file.is_open()) {
                m_lastError = "Failed to open project file: " + projectFile.string();
                return false;
            }

            nlohmann::json projectJson;
            file >> projectJson;
            file.close();

            if (projectJson.contains("settings")) {
                m_projectSettings.Deserialize(projectJson["settings"]);
            }
            else {
                m_lastError = "Invalid project file format: missing settings";
                return false;
            }

            m_currentProjectPath = projectPath;
            m_isProjectOpen = true;

            UpdateProjectSpecificPaths();

            auto fileSys = GetFilePathSystem();
            if (fileSys) {
                fileSys->SetPath("LastOpenProject", projectPath);
            }

            AddToRecentProjects(projectPath);

            std::cout << "[ProjectManager] Loaded project: " << m_projectSettings.projectName << " from " << projectPath << std::endl;
            if (fileSys) {
                std::cout << "  - AssetsFolder: " << fileSys->GetPath("AssetsFolder") << std::endl;
                std::cout << "  - OutputFolder: " << fileSys->GetPath("OutputFolder") << std::endl;
            }

            if (m_onProjectLoadedCallback) {
                m_onProjectLoadedCallback(projectPath);
            }

            LoadViewState();

            auto allWorkspaces = m_viewManager.GetAllWorkspaces();
            if (allWorkspaces.empty()) {
                std::cout << "[ProjectManager] No workspaces found, creating default workspace" << std::endl;
                GUI::WorkspaceID defaultWorkspace = m_viewManager.CreateView();
                m_viewState.SetLastActiveWorkspace(defaultWorkspace);
                m_viewManager.SetActiveWorkspace(defaultWorkspace);
                std::cout << "[ProjectManager] Created default workspace: " << defaultWorkspace << std::endl;
            }
            else {
                std::cout << "[ProjectManager] Loaded " << allWorkspaces.size() << " workspaces" << std::endl;
            }

            LoadImGuiLayout();
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
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            m_projectSettings.lastModified = ss.str();

            std::filesystem::path projectFile = std::filesystem::path(m_currentProjectPath) / "project.ani";
            std::ofstream file(projectFile);
            if (!file.is_open()) {
                m_lastError = "Failed to open project file for writing: " + projectFile.string();
                return false;
            }

            nlohmann::json projectJson;
            projectJson["version"] = "1.0";
            projectJson["settings"] = m_projectSettings.Serialize();
            file << projectJson.dump(4);
            file.close();

            SaveViewState();
            SaveImGuiLayout();
            SaveProjectWindowState();

            UpdateProjectSpecificPaths();

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

        std::cout << "[ProjectManager] CloseProject() called" << std::endl;
        std::cout << "[ProjectManager] Closing project: " << m_projectSettings.projectName << std::endl;

        SaveProject();

        m_viewManager.Reset();

        ClearProjectSpecificPaths();

        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("LastOpenProject", "");
        }

        m_isProjectOpen = false;
        m_currentProjectPath.clear();
        m_projectSettings = ProjectSettings{};
        m_viewState.Reset();

        std::cout << "[ProjectManager] Project closed" << std::endl;

        if (m_onProjectClosedCallback) {
            m_onProjectClosedCallback();
        }
    }

    bool ProjectManager::IsProjectNameTaken(const std::string& projectName, const std::string& excludePath) const {
        std::string defaultPath = GetDefaultProjectPath();
        if (defaultPath.empty()) return false;

        std::filesystem::path projectDir = std::filesystem::path(defaultPath) / projectName;

        if (!excludePath.empty() && std::filesystem::equivalent(projectDir, excludePath)) {
            return false;
        }

        return std::filesystem::exists(projectDir);
    }

    std::vector<std::string> ProjectManager::GetRecentProjects() const {
        std::vector<std::string> recentProjects;
        std::string defaultPath = GetDefaultProjectPath();
        if (defaultPath.empty()) {
            std::cerr << "[ProjectManager] ERROR: DefaultProject path is empty!" << std::endl;
            return recentProjects;
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(defaultPath)) {
                if (entry.is_directory()) {
                    std::string projectFile = entry.path().string() + "/project.ani";
                    if (std::filesystem::exists(projectFile)) {
                        recentProjects.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectManager] Error scanning for projects: " << e.what() << std::endl;
        }

        return recentProjects;
    }

    void ProjectManager::AddToRecentProjects(const std::string& projectPath) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("LastOpenProject", projectPath);
        }
    }

    void ProjectManager::SetDefaultProjectPath(const std::string& path) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("DefaultProject", path);
        }
    }

    std::string ProjectManager::GetDefaultProjectPath() const {
        auto fileSys = GetFilePathSystem();
        return fileSys ? fileSys->GetPath("DefaultProject") : "";
    }

    void ProjectManager::SetAssetsFolder(const std::string& path) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("AssetsFolder", path);
        }
    }

    std::string ProjectManager::GetAssetsFolder() const {
        auto fileSys = GetFilePathSystem();
        return fileSys ? fileSys->GetPath("AssetsFolder") : "";
    }

    void ProjectManager::SetOutputFolder(const std::string& path) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("OutputFolder", path);
        }
    }

    std::string ProjectManager::GetOutputFolder() const {
        auto fileSys = GetFilePathSystem();
        return fileSys ? fileSys->GetPath("OutputFolder") : "";
    }

    bool ProjectManager::ApplyProjectTemplate(const GUI::ProjectTemplate& template_) {
        if (!m_isProjectOpen) {
            m_lastError = "No project is currently open";
            return false;
        }

        try {
            std::cout << "[ProjectManager] Applying project template: " << template_.name << std::endl;

            GUI::WorkspaceID currentWorkspace = m_viewState.GetLastActiveWorkspace();
            if (!template_.name.empty()) {
                m_viewManager.SetWorkspaceName(currentWorkspace, template_.name);
            }

            for (const auto& viewTypeName : template_.defaultOpenViews) {
                try {
                    std::cout << "[ProjectManager] Adding view: " << viewTypeName << " to workspace: " << currentWorkspace << std::endl;
                    GUI::ViewTypeID viewType = m_viewManager.GetViewType(viewTypeName);
                    m_viewManager.AddViewByType(currentWorkspace, viewType);
                    std::cout << "[ProjectManager] Successfully added view: " << viewTypeName << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[ProjectManager] Failed to add view " << viewTypeName << ": " << e.what() << std::endl;
                }
            }

            if (!template_.settings.empty()) {
                std::cout << "[ProjectManager] Template has settings (not implemented yet)" << std::endl;
            }

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
                GUI::WorkspaceID lastActiveWorkspace = m_viewState.GetLastActiveWorkspace();
                auto allWorkspaces = m_viewManager.GetAllWorkspaces();

                if (std::find(allWorkspaces.begin(), allWorkspaces.end(), lastActiveWorkspace) == allWorkspaces.end()) {
                    if (!allWorkspaces.empty()) {
                        lastActiveWorkspace = allWorkspaces[0];
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        std::cout << "[ProjectManager] Corrected active workspace to: " << lastActiveWorkspace << std::endl;
                    }
                    else {
                        lastActiveWorkspace = m_viewManager.CreateView();
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        std::cout << "[ProjectManager] Created default workspace: " << lastActiveWorkspace << std::endl;
                    }
                }

                m_viewManager.SetActiveWorkspace(lastActiveWorkspace);
                std::cout << "[ProjectManager] Loaded ViewState with active workspace: " << lastActiveWorkspace << std::endl;

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
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectManager] Exception saving ImGui layout: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectManager::LoadImGuiLayout() {
        try {
            Utils::ImGuiStateUtils::LoadProjectImGuiLayout(m_currentProjectPath);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectManager] Exception loading ImGui layout: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectManager::SaveProjectWindowState() {
        if (!m_windowHandle) return false;

        try {
            Utils::WindowState windowState;
            windowState.SetGlobalDataPath(GetProjectDataPath());

            GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_windowHandle);
            int width, height, x, y;
            glfwGetWindowSize(glfwWindow, &width, &height);
            glfwGetWindowPos(glfwWindow, &x, &y);

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
                return false;
            }

            Utils::WindowState windowState;
            windowState.SetGlobalDataPath(GetProjectDataPath());

            if (windowState.LoadFromFile(windowStatePath)) {
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
        if (!m_isProjectOpen) return;

        std::string assetsPath = GetProjectAssetsPath();
        std::string outputPath = GetProjectOutputPath();
        std::string dataPath = GetProjectDataPath();

        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            if (!assetsPath.empty()) fileSys->SetPath("AssetsFolder", assetsPath);
            if (!outputPath.empty()) fileSys->SetPath("OutputFolder", outputPath);
            fileSys->SetPath("ProjectDataPath", dataPath);
        }

        std::cout << "[ProjectManager] Updated project-specific paths:" << std::endl;
        std::cout << "  - AssetsFolder: " << assetsPath << std::endl;
        std::cout << "  - OutputFolder: " << outputPath << std::endl;
        std::cout << "  - ProjectDataPath: " << dataPath << std::endl;
    }

    void ProjectManager::ClearProjectSpecificPaths() {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("AssetsFolder", "");
            fileSys->SetPath("OutputFolder", "");
            fileSys->SetPath("ProjectDataPath", "");
        }

        std::cout << "[ProjectManager] Cleared project-specific paths" << std::endl;
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