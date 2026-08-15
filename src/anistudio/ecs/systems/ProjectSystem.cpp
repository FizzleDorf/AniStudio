// ProjectSystem.cpp
#include "ProjectSystem.hpp"
#include "ViewManager.hpp"
#include "WindowState.hpp"
#include "ImGuiStateUtils.hpp"
#include "FilePathSystem.hpp"
#include "Events.hpp"
#include "ProjectTemplate.hpp"
#include "GeneralSettingsComponent.hpp"
#include "SettingsSystem.hpp"
#include "StudioPluginManager.hpp"
#include <GLFW/glfw3.h>
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

    ProjectSystem::ProjectSystem(ECS::EntityManager& mgr)
        : ECS::BaseSystem(mgr)
        , m_projectEntity(0)
        , m_componentTypeId(ECS::MAX_COMPONENT_COUNT)
        , m_viewManager(nullptr)
        , m_pluginManager(nullptr)
        , m_windowHandle(nullptr) {
        sysName = "ProjectSystem";
    }

    void ProjectSystem::Start() {
        m_componentTypeId = mgr.RegisterComponent<ProjectComponent>("ProjectComponent");
        m_projectEntity = mgr.AddNewEntity();
        mgr.AddComponent<ProjectComponent>(m_projectEntity);
        std::cout << "[ProjectSystem] Started with entity " << m_projectEntity << std::endl;
    }

    void ProjectSystem::Destroy() {
        if (mgr.IsEntityValid(m_projectEntity)) {
            mgr.DestroyEntity(m_projectEntity);
            m_projectEntity = 0;
        }
    }

    ProjectComponent* ProjectSystem::GetProjectComponent() const {
        if (!mgr.IsEntityValid(m_projectEntity)) return nullptr;
        auto* base = mgr.GetComponentById(m_projectEntity, m_componentTypeId);
        return dynamic_cast<ProjectComponent*>(base);
    }

    void ProjectSystem::SetWindowHandle(void* windowHandle) {
        m_windowHandle = windowHandle;
        std::cout << "[ProjectSystem] Window handle set" << std::endl;
    }

    void ProjectSystem::SetViewManager(GUI::ViewManager* viewManager) {
        m_viewManager = viewManager;
        std::cout << "[ProjectSystem] ViewManager set" << std::endl;
    }

    void ProjectSystem::SetPluginManager(Plugins::StudioPluginManager* pluginManager) {
        m_pluginManager = pluginManager;
        std::cout << "[ProjectSystem] PluginManager set" << std::endl;
    }

    std::shared_ptr<ECS::FilePathSystem> ProjectSystem::GetFilePathSystem() const {
        return mgr.GetSystem<ECS::FilePathSystem>();
    }

    std::string ProjectSystem::GetDefaultProjectPath() const {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            return fileSys->GetPath("DefaultProject");
        }
        return "";
    }

    void ProjectSystem::SetDefaultProjectPath(const std::string& path) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("DefaultProject", path);
        }
    }

    std::string ProjectSystem::GenerateDefaultProjectName() const {
        std::string baseName = "AniProject";
        std::string defaultPath = GetDefaultProjectPath();

        if (defaultPath.empty()) {
            return baseName + "1";
        }

        int counter = 1;
        std::string candidateName;

        do {
            candidateName = baseName + std::to_string(counter);
            counter++;
            if (counter > 9999) {
                candidateName = baseName + "_" + std::to_string(std::time(nullptr));
                break;
            }
        } while (IsProjectNameTaken(candidateName));

        return candidateName;
    }

    bool ProjectSystem::ShouldShowStartup() const {
        std::cout << "[ProjectSystem] ShouldShowStartup check:" << std::endl;
        std::cout << "  - Project open: " << (IsProjectOpen() ? "YES" : "NO") << std::endl;

        if (IsProjectOpen()) {
            std::cout << "  - Project already open" << std::endl;
            return false;
        }

        auto fileSys = GetFilePathSystem();
        std::string lastProjectPath;
        if (fileSys) {
            lastProjectPath = fileSys->GetPath("LastOpenProject");
        }

        bool loadLastProject = true;
        auto settingsSystem = mgr.GetSystem<ECS::SettingsSystem>();
        if (settingsSystem) {
            ECS::EntityID settingsEntity = settingsSystem->GetSettingsEntity();
            if (mgr.IsEntityValid(settingsEntity) && mgr.HasComponent<ECS::GeneralSettingsComponent>(settingsEntity)) {
                auto& generalComp = mgr.GetComponent<ECS::GeneralSettingsComponent>(settingsEntity);
                loadLastProject = generalComp.loadLastProject;
                std::cout << "  - loadLastProject setting: " << (loadLastProject ? "YES" : "NO") << std::endl;
            }
        }

        if (!loadLastProject) {
            std::cout << "  - loadLastProject is disabled, showing startup" << std::endl;
            return true;
        }

        if (!lastProjectPath.empty() && std::filesystem::exists(lastProjectPath)) {
            std::cout << "  - Has last opened project: " << lastProjectPath << std::endl;
            const_cast<ProjectSystem*>(this)->LoadProject(lastProjectPath);
            return false;
        }

        std::cout << "  - No last opened project" << std::endl;
        std::cout << "  - Should show startup: YES" << std::endl;
        return true;
    }

    bool ProjectSystem::IsProjectOpen() const {
        auto* comp = GetProjectComponent();
        return comp && comp->isOpen;
    }

    const std::string& ProjectSystem::GetCurrentProjectPath() const {
        static std::string empty;
        auto* comp = GetProjectComponent();
        return comp ? comp->currentProjectPath : empty;
    }

    const std::string& ProjectSystem::GetCurrentProjectName() const {
        static std::string empty;
        auto* comp = GetProjectComponent();
        return comp ? comp->settings.projectName : empty;
    }

    std::string ProjectSystem::GetProjectDataPath() const {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) return "";
        return comp->currentProjectPath + "/data";
    }

    GUI::ViewState& ProjectSystem::GetViewState() {
        return m_viewState;
    }

    const GUI::ViewState& ProjectSystem::GetViewState() const {
        return m_viewState;
    }

    void ProjectSystem::SetLastActiveWorkspace(GUI::WorkspaceID workspaceID) {
        m_viewState.SetLastActiveWorkspace(workspaceID);
    }

    GUI::WorkspaceID ProjectSystem::GetLastActiveWorkspace() const {
        return m_viewState.GetLastActiveWorkspace();
    }

    void ProjectSystem::SetProjectLoadedCallback(std::function<void(const std::string&)> callback) {
        m_onProjectLoadedCallback = callback;
    }

    void ProjectSystem::SetProjectCreatedCallback(std::function<void(const std::string&)> callback) {
        m_onProjectCreatedCallback = callback;
    }

    void ProjectSystem::SetProjectClosedCallback(std::function<void()> callback) {
        m_onProjectClosedCallback = callback;
    }

    void ProjectSystem::SetViewStateLoadedCallback(std::function<void(GUI::WorkspaceID)> callback) {
        m_onViewStateLoadedCallback = callback;
    }

    bool ProjectSystem::IsProjectNameTaken(const std::string& projectName, const std::string& excludePath) const {
        std::string defaultPath = GetDefaultProjectPath();
        if (defaultPath.empty()) return false;

        std::filesystem::path projectDir = std::filesystem::path(defaultPath) / projectName;

        if (!excludePath.empty() && std::filesystem::equivalent(projectDir, excludePath)) {
            return false;
        }

        return std::filesystem::exists(projectDir);
    }

    std::vector<std::string> ProjectSystem::GetRecentProjects() const {
        std::vector<std::string> recentProjects;
        std::string defaultPath = GetDefaultProjectPath();
        if (defaultPath.empty()) {
            std::cerr << "[ProjectSystem] ERROR: DefaultProject path is empty!" << std::endl;
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
            std::cerr << "[ProjectSystem] Error scanning for projects: " << e.what() << std::endl;
        }

        return recentProjects;
    }

    void ProjectSystem::AddToRecentProjects(const std::string& projectPath) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("LastOpenProject", projectPath);
        }
    }

    bool ProjectSystem::CreateNewProject(const std::string& projectPath, const std::string& projectName) {
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

            auto* comp = GetProjectComponent();
            if (!comp) return false;

            comp->isOpen = true;
            comp->currentProjectPath = projectPath;
            comp->settings = ProjectSettings{};
            comp->settings.projectName = projectName;
            comp->settings.projectVersion = "1.0.0";

            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            comp->settings.createdDate = ss.str();
            comp->settings.lastModified = ss.str();

            UpdateProjectSpecificPaths();

            auto fileSys = GetFilePathSystem();
            if (fileSys) {
                fileSys->SetPath("LastOpenProject", projectPath);
            }

            m_viewState.Reset();
            if (m_viewManager) {
                GUI::WorkspaceID defaultWorkspace = m_viewManager->CreateView();
                m_viewState.SetLastActiveWorkspace(defaultWorkspace);
                m_viewManager->SetActiveWorkspace(defaultWorkspace);
                std::cout << "[ProjectSystem] Created default workspace: " << defaultWorkspace << std::endl;
            }

            if (!SaveProject()) {
                m_lastError = "Failed to save new project";
                comp->isOpen = false;
                comp->currentProjectPath.clear();
                return false;
            }

            AddToRecentProjects(projectPath);

            if (m_pluginManager) {
                m_pluginManager->LoadStagingPlugins(true);
                std::cout << "[ProjectSystem] Processed staging plugins for loaded project" << std::endl;
            }

            std::cout << "[ProjectSystem] Created new project: " << projectName << " at " << projectPath << std::endl;
            if (m_onProjectCreatedCallback) {
                m_onProjectCreatedCallback(projectPath);
            }
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception creating project: " + std::string(e.what());
            std::cerr << "[ProjectSystem] " << m_lastError << std::endl;
            return false;
        }
    }

    bool ProjectSystem::LoadProject(const std::string& projectPath) {
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

            if (IsProjectOpen()) {
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

            auto* comp = GetProjectComponent();
            if (!comp) return false;

            if (projectJson.contains("settings")) {
                comp->settings.Deserialize(projectJson["settings"]);
            }
            else {
                m_lastError = "Invalid project file format: missing settings";
                return false;
            }

            comp->isOpen = true;
            comp->currentProjectPath = projectPath;

            UpdateProjectSpecificPaths();

            auto fileSys = GetFilePathSystem();
            if (fileSys) {
                fileSys->SetPath("LastOpenProject", projectPath);
            }

            AddToRecentProjects(projectPath);

            std::cout << "[ProjectSystem] Loaded project: " << comp->settings.projectName << " from " << projectPath << std::endl;
            if (fileSys) {
                std::cout << "  - AssetsFolder: " << fileSys->GetPath("AssetsFolder") << std::endl;
                std::cout << "  - OutputFolder: " << fileSys->GetPath("OutputFolder") << std::endl;
            }

            if (m_onProjectLoadedCallback) {
                m_onProjectLoadedCallback(projectPath);
            }

            LoadViewState();

            if (m_viewManager) {
                auto allWorkspaces = m_viewManager->GetAllWorkspaces();
                if (allWorkspaces.empty()) {
                    std::cout << "[ProjectSystem] No workspaces found, creating default workspace" << std::endl;
                    GUI::WorkspaceID defaultWorkspace = m_viewManager->CreateView();
                    m_viewState.SetLastActiveWorkspace(defaultWorkspace);
                    m_viewManager->SetActiveWorkspace(defaultWorkspace);
                    std::cout << "[ProjectSystem] Created default workspace: " << defaultWorkspace << std::endl;
                }
                else {
                    std::cout << "[ProjectSystem] Loaded " << allWorkspaces.size() << " workspaces" << std::endl;
                }
            }

            LoadImGuiLayout();
            LoadAndApplyProjectWindowState();

            UpdateProjectSpecificPaths();

            if (m_pluginManager) {
                m_pluginManager->LoadStagingPlugins(true);
                std::cout << "[ProjectSystem] Processed staging plugins for loaded project" << std::endl;
            }

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception loading project: " + std::string(e.what());
            std::cerr << "[ProjectSystem] " << m_lastError << std::endl;
            return false;
        }
    }

    bool ProjectSystem::SaveProject() {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) {
            m_lastError = "No project is currently open";
            return false;
        }

        m_lastError.clear();
        try {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            comp->settings.lastModified = ss.str();

            std::filesystem::path projectFile = std::filesystem::path(comp->currentProjectPath) / "project.ani";
            std::ofstream file(projectFile);
            if (!file.is_open()) {
                m_lastError = "Failed to open project file for writing: " + projectFile.string();
                return false;
            }

            nlohmann::json projectJson;
            projectJson["version"] = "1.0";
            projectJson["settings"] = comp->settings.Serialize();
            file << projectJson.dump(4);
            file.close();

            SaveViewState();
            SaveImGuiLayout();
            SaveProjectWindowState();

            UpdateProjectSpecificPaths();

            std::cout << "[ProjectSystem] Project saved: " << comp->settings.projectName << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception saving project: " + std::string(e.what());
            std::cerr << "[ProjectSystem] " << m_lastError << std::endl;
            return false;
        }
    }

    void ProjectSystem::CloseProject() {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) {
            std::cout << "[ProjectSystem] No project to close" << std::endl;
            return;
        }

        std::cout << "[ProjectSystem] CloseProject() called" << std::endl;
        std::cout << "[ProjectSystem] Closing project: " << comp->settings.projectName << std::endl;

        SaveProject();

        if (m_viewManager) {
            m_viewManager->Reset();
        }

        ClearProjectSpecificPaths();

        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("LastOpenProject", "");
        }

        comp->isOpen = false;
        comp->currentProjectPath.clear();
        comp->settings = ProjectSettings{};
        m_viewState.Reset();

        std::cout << "[ProjectSystem] Project closed" << std::endl;

        if (m_onProjectClosedCallback) {
            m_onProjectClosedCallback();
        }
    }

    bool ProjectSystem::ApplyProjectTemplate(const GUI::ProjectTemplate& template_) {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) {
            m_lastError = "No project is currently open";
            return false;
        }

        try {
            std::cout << "[ProjectSystem] Applying project template: " << template_.name << std::endl;

            GUI::WorkspaceID currentWorkspace = m_viewState.GetLastActiveWorkspace();
            if (m_viewManager) {
                if (!template_.name.empty()) {
                    m_viewManager->SetWorkspaceName(currentWorkspace, template_.name);
                }

                for (const auto& viewTypeName : template_.defaultOpenViews) {
                    try {
                        std::cout << "[ProjectSystem] Adding view: " << viewTypeName << " to workspace: " << currentWorkspace << std::endl;
                        GUI::ViewTypeID viewType = m_viewManager->GetViewType(viewTypeName);
                        m_viewManager->AddViewByType(currentWorkspace, viewType);
                        std::cout << "[ProjectSystem] Successfully added view: " << viewTypeName << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[ProjectSystem] Failed to add view " << viewTypeName << ": " << e.what() << std::endl;
                    }
                }
            }

            if (!template_.settings.empty()) {
                std::cout << "[ProjectSystem] Template has settings (not implemented yet)" << std::endl;
            }

            SaveProject();
            std::cout << "[ProjectSystem] Successfully applied template: " << template_.name << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception applying project template: " + std::string(e.what());
            std::cerr << "[ProjectSystem] " << m_lastError << std::endl;
            return false;
        }
    }

    bool ProjectSystem::SaveViewState() {
        if (!m_viewManager) return false;
        try {
            std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";
            std::cout << "[ProjectSystem] Saving ViewState with active workspace: " << m_viewState.GetLastActiveWorkspace() << std::endl;
            return m_viewState.SaveViewManagerState(*m_viewManager, viewStatePath);
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectSystem] Exception saving ViewState: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectSystem::LoadViewState() {
        if (!m_viewManager) return false;

        try {
            std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";
            bool success = m_viewState.LoadViewManagerState(*m_viewManager, viewStatePath);

            if (success) {
                GUI::WorkspaceID lastActiveWorkspace = m_viewState.GetLastActiveWorkspace();
                auto allWorkspaces = m_viewManager->GetAllWorkspaces();

                if (std::find(allWorkspaces.begin(), allWorkspaces.end(), lastActiveWorkspace) == allWorkspaces.end()) {
                    if (!allWorkspaces.empty()) {
                        lastActiveWorkspace = allWorkspaces[0];
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        std::cout << "[ProjectSystem] Corrected active workspace to: " << lastActiveWorkspace << std::endl;
                    }
                    else {
                        lastActiveWorkspace = m_viewManager->CreateView();
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        std::cout << "[ProjectSystem] Created default workspace: " << lastActiveWorkspace << std::endl;
                    }
                }

                m_viewManager->SetActiveWorkspace(lastActiveWorkspace);
                std::cout << "[ProjectSystem] Loaded ViewState with active workspace: " << lastActiveWorkspace << std::endl;

                if (m_onViewStateLoadedCallback) {
                    m_onViewStateLoadedCallback(lastActiveWorkspace);
                }
            }

            return success;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectSystem] Exception loading ViewState: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectSystem::SaveImGuiLayout() {
        try {
            auto* comp = GetProjectComponent();
            if (!comp || !comp->isOpen) return false;
            Utils::ImGuiStateUtils::SaveProjectImGuiLayout(comp->currentProjectPath);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectSystem] Exception saving ImGui layout: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectSystem::LoadImGuiLayout() {
        try {
            auto* comp = GetProjectComponent();
            if (!comp || !comp->isOpen) return false;
            Utils::ImGuiStateUtils::LoadProjectImGuiLayout(comp->currentProjectPath);
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectSystem] Exception loading ImGui layout: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectSystem::SaveProjectWindowState() {
        if (!m_windowHandle) return false;

        try {
            auto* comp = GetProjectComponent();
            if (!comp || !comp->isOpen) return false;

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
            std::cerr << "[ProjectSystem] Exception saving window state: " << e.what() << std::endl;
            return false;
        }
    }

    bool ProjectSystem::LoadAndApplyProjectWindowState() {
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

                std::cout << "[ProjectSystem] Applied project window state" << std::endl;
                return true;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[ProjectSystem] Exception loading window state: " << e.what() << std::endl;
        }

        return false;
    }

    void ProjectSystem::UpdateProjectSpecificPaths() {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) return;

        std::string assetsPath = GetProjectAssetsPath();
        std::string outputPath = GetProjectOutputPath();
        std::string dataPath = GetProjectDataPath();

        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            if (!assetsPath.empty()) fileSys->SetPath("AssetsFolder", assetsPath);
            if (!outputPath.empty()) fileSys->SetPath("OutputFolder", outputPath);
            fileSys->SetPath("ProjectDataPath", dataPath);
        }

        std::cout << "[ProjectSystem] Updated project-specific paths:" << std::endl;
        std::cout << "  - AssetsFolder: " << assetsPath << std::endl;
        std::cout << "  - OutputFolder: " << outputPath << std::endl;
        std::cout << "  - ProjectDataPath: " << dataPath << std::endl;
    }

    void ProjectSystem::ClearProjectSpecificPaths() {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("AssetsFolder", "");
            fileSys->SetPath("OutputFolder", "");
            fileSys->SetPath("ProjectDataPath", "");
        }

        std::cout << "[ProjectSystem] Cleared project-specific paths" << std::endl;
    }

    std::string ProjectSystem::GetProjectAssetsPath() const {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) return "";
        return comp->currentProjectPath + "/assets";
    }

    std::string ProjectSystem::GetProjectOutputPath() const {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) return "";
        return comp->currentProjectPath + "/output";
    }

    std::string ProjectSystem::GetProjectWindowStatePath() const {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) return "";
        return GetProjectDataPath() + "/window_state.json";
    }

} // namespace ANI