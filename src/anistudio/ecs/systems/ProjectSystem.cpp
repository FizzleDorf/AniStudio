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
#include "Log.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <set>
#include <algorithm>

namespace ECS {

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
        , m_windowHandle(nullptr)
        , m_autoSaveTimer(0.0f)
        , m_autoSaveEnabled(true)
        , m_autoSaveIntervalMinutes(5) {
        sysName = "ProjectSystem";
    }

    void ProjectSystem::Start() {
        m_componentTypeId = mgr.RegisterComponent<ProjectComponent>("ProjectComponent");
        m_projectEntity = mgr.AddNewEntity();
        mgr.AddComponent<ProjectComponent>(m_projectEntity);
        UpdateAutoSaveSettings();
        ANI_LOG_INFO("[ProjectSystem] Started with entity %u", (unsigned)m_projectEntity);
    }

    void ProjectSystem::Destroy() {
        if (mgr.IsEntityValid(m_projectEntity)) {
            mgr.DestroyEntity(m_projectEntity);
            m_projectEntity = 0;
        }
    }

    void ProjectSystem::Update(float deltaT) {
        if (!IsProjectOpen()) return;
        if (!m_autoSaveEnabled) return;

        m_autoSaveTimer += deltaT;
        float intervalSeconds = static_cast<float>(m_autoSaveIntervalMinutes) * 60.0f;
        if (m_autoSaveTimer >= intervalSeconds) {
            SaveProject();
            m_autoSaveTimer = 0.0f;
            ANI_LOG_INFO("[ProjectSystem] Auto-saved project");
        }
    }

    ProjectComponent* ProjectSystem::GetProjectComponent() const {
        if (!mgr.IsEntityValid(m_projectEntity)) return nullptr;
        auto* base = mgr.GetComponentById(m_projectEntity, m_componentTypeId);
        return dynamic_cast<ProjectComponent*>(base);
    }

    void ProjectSystem::SetWindowHandle(void* windowHandle) {
        m_windowHandle = windowHandle;
        ANI_LOG_INFO("[ProjectSystem] Window handle set");
    }

    void ProjectSystem::SetViewManager(GUI::ViewManager* viewManager) {
        m_viewManager = viewManager;

        if (m_viewManager) {
            m_viewManager->SetViewClosingCallback(
                [this](GUI::WorkspaceID ws, GUI::ViewTypeID type,
                    const std::string& name, const nlohmann::json& state) {
                        HandleViewClosing(ws, type, name, state);
                });
            m_viewManager->SetViewClosedCallback(
                [this](GUI::WorkspaceID ws, GUI::ViewTypeID type,
                    const std::string& name) {
                        HandleViewClosed(ws, type, name);
                });
            m_viewManager->SetViewOpeningCallback(
                [this](GUI::WorkspaceID ws, GUI::ViewTypeID type,
                    const std::string& name) {
                        HandleViewOpening(ws, type, name);
                });
            m_viewManager->SetViewOpenedCallback(
                [this](GUI::WorkspaceID ws, GUI::ViewTypeID type,
                    const std::string& name) {
                        HandleViewOpened(ws, type, name);
                });
        }

        ANI_LOG_INFO("[ProjectSystem] ViewManager set and callbacks registered");
    }

    void ProjectSystem::SetPluginManager(Plugins::StudioPluginManager* pluginManager) {
        m_pluginManager = pluginManager;
        ANI_LOG_INFO("[ProjectSystem] PluginManager set");
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

    std::string ProjectSystem::GetCaptureDirectory() const {
        std::string data = GetProjectDataPath();
        if (data.empty()) return "";
        return data + "/capture";
    }

    std::string ProjectSystem::GetQuicksaveDirectory() const {
        std::string data = GetProjectDataPath();
        if (data.empty()) return "";
        return data + "/quicksaves";
    }

    std::string ProjectSystem::GetCapturePath(const std::string& viewName, GUI::WorkspaceID workspaceID) const {
        std::string dir = GetCaptureDirectory();
        if (dir.empty()) return "";
        return dir + "/" + viewName + "_" + std::to_string((unsigned)workspaceID) + ".json";
    }

    std::string ProjectSystem::GetQuicksavePath(const std::string& viewName, GUI::WorkspaceID workspaceID) const {
        std::string dir = GetQuicksaveDirectory();
        if (dir.empty()) return "";
        return dir + "/" + viewName + "_" + std::to_string((unsigned)workspaceID) + ".json";
    }

    bool ProjectSystem::WriteJsonFile(const std::string& path, const nlohmann::json& j) const {
        if (path.empty()) return false;
        try {
            std::filesystem::path p(path);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
            std::ofstream file(path);
            if (!file.is_open()) {
                ANI_LOG_ERROR("[ProjectSystem] Failed to open %s for writing", path.c_str());
                return false;
            }
            file << j.dump(4);
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ProjectSystem] Exception writing %s: %s", path.c_str(), e.what());
            return false;
        }
    }

    bool ProjectSystem::ReadJsonFile(const std::string& path, nlohmann::json& out) const {
        try {
            std::ifstream file(path);
            if (!file.is_open()) return false;
            file >> out;
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ProjectSystem] Exception reading %s: %s", path.c_str(), e.what());
            return false;
        }
    }

    void ProjectSystem::HandleViewClosing(GUI::WorkspaceID workspaceID, GUI::ViewTypeID /*viewType*/,
        const std::string& viewName, const nlohmann::json& state) {
        if (m_suppressViewStateSave) return;
        if (!IsProjectOpen()) return;
        if (viewName.empty()) {
            ANI_LOG_WARN("[ProjectSystem] ViewClosing with empty viewName, skipping capture");
            return;
        }

        std::string path = GetCapturePath(viewName, workspaceID);
        if (path.empty()) {
            ANI_LOG_WARN("[ProjectSystem] ViewClosing: no capture path (project data dir missing)");
            return;
        }

        nlohmann::json wrapper;
        wrapper["viewName"] = viewName;
        wrapper["workspaceID"] = (unsigned)workspaceID;
        wrapper["state"] = state;

        if (WriteJsonFile(path, wrapper)) {
            ANI_LOG_INFO("[ProjectSystem] Captured view state: %s", path.c_str());
        }
    }

    void ProjectSystem::HandleViewClosed(GUI::WorkspaceID workspaceID, GUI::ViewTypeID /*viewType*/,
        const std::string& viewName) {
        ANI_LOG_INFO("[ProjectSystem] View closed: %s in workspace %u",
            viewName.c_str(), (unsigned)workspaceID);
        if (m_suppressViewStateSave) return;
        if (IsProjectOpen()) {
            SaveViewState();
        }
    }

    void ProjectSystem::HandleViewOpening(GUI::WorkspaceID workspaceID, GUI::ViewTypeID /*viewType*/,
        const std::string& viewName) {
        ANI_LOG_INFO("[ProjectSystem] View opening: %s in workspace %u",
            viewName.c_str(), (unsigned)workspaceID);
    }

    void ProjectSystem::HandleViewOpened(GUI::WorkspaceID workspaceID, GUI::ViewTypeID /*viewType*/,
        const std::string& viewName) {
        ANI_LOG_INFO("[ProjectSystem] View opened: %s in workspace %u",
            viewName.c_str(), (unsigned)workspaceID);

        if (m_suppressCaptureApply) return;

        if (!IsProjectOpen() || !m_viewManager) return;
        if (viewName.empty()) return;

        std::string path = GetCapturePath(viewName, workspaceID);
        if (path.empty()) return;

        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            return;
        }

        nlohmann::json wrapper;
        if (!ReadJsonFile(path, wrapper) || !wrapper.contains("state")) {
            ANI_LOG_WARN("[ProjectSystem] Capture file invalid, leaving on disk: %s", path.c_str());
            return;
        }

        GUI::ViewTypeID viewType;
        try {
            viewType = m_viewManager->GetViewType(viewName);
        }
        catch (const std::exception&) {
            ANI_LOG_WARN("[ProjectSystem] Capture refers to unregistered view '%s', leaving file",
                viewName.c_str());
            return;
        }

        auto& wsMap = m_viewManager->GetWorkspaces();
        auto wsIt = wsMap.find(workspaceID);
        if (wsIt == wsMap.end()) {
            ANI_LOG_WARN("[ProjectSystem] Capture references unknown workspace %u, leaving file",
                (unsigned)workspaceID);
            return;
        }

        auto vIt = wsIt->second.find(viewType);
        if (vIt == wsIt->second.end() || !vIt->second) {
            ANI_LOG_WARN("[ProjectSystem] View instance not present after open for %s, leaving file",
                viewName.c_str());
            return;
        }

        try {
            vIt->second->Deserialize(wrapper["state"]);
            ANI_LOG_INFO("[ProjectSystem] Applied captured state for %s in workspace %u",
                viewName.c_str(), (unsigned)workspaceID);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ProjectSystem] Failed to apply capture for %s: %s",
                viewName.c_str(), e.what());
            return;
        }

        std::error_code rmEc;
        if (std::filesystem::remove(path, rmEc)) {
            ANI_LOG_INFO("[ProjectSystem] Consumed capture file: %s", path.c_str());
        }
        else {
            ANI_LOG_WARN("[ProjectSystem] Failed to remove capture file: %s", path.c_str());
        }
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
            ANI_LOG_ERROR("[ProjectSystem] DefaultProject path is empty!");
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
            ANI_LOG_ERROR("[ProjectSystem] Error scanning for projects: %s", e.what());
        }

        return recentProjects;
    }

    void ProjectSystem::AddToRecentProjects(const std::string& projectPath) {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("LastOpenProject", projectPath);
        }
    }

    void ProjectSystem::UpdateAutoSaveSettings() {
        auto settingsSystem = mgr.GetSystem<ECS::SettingsSystem>();
        if (settingsSystem) {
            ECS::EntityID settingsEntity = settingsSystem->GetSettingsEntity();
            if (mgr.IsEntityValid(settingsEntity) && mgr.HasComponent<ECS::GeneralSettingsComponent>(settingsEntity)) {
                auto& generalComp = mgr.GetComponent<ECS::GeneralSettingsComponent>(settingsEntity);
                m_autoSaveEnabled = generalComp.autoSaveProjects;
                m_autoSaveIntervalMinutes = generalComp.autoSaveIntervalMinutes;
                ANI_LOG_INFO("[ProjectSystem] Auto-save settings updated: enabled=%d, interval=%d minutes",
                    m_autoSaveEnabled ? 1 : 0, m_autoSaveIntervalMinutes);
            }
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
            std::filesystem::create_directories(projPath / "data" / "capture");
            std::filesystem::create_directories(projPath / "data" / "quicksaves");
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
                ANI_LOG_INFO("[ProjectSystem] Created default workspace: %u", (unsigned)defaultWorkspace);
            }

            if (!SaveProject()) {
                m_lastError = "Failed to save new project";
                comp->isOpen = false;
                comp->currentProjectPath.clear();
                return false;
            }

            AddToRecentProjects(projectPath);

            // Process staged plugins and set project context BEFORE firing the
            // created callback so plugin view types are available for any
            // subsequent view/layout setup.
            if (m_pluginManager) {
                m_pluginManager->LoadStagingPlugins(true);
                m_pluginManager->SetProjectContext(projectPath);
                m_pluginManager->PrepareProjectPlugins();
                ANI_LOG_INFO("[ProjectSystem] Processed plugins for created project");
            }

            UpdateAutoSaveSettings();

            ANI_LOG_INFO("[ProjectSystem] Created new project: %s at %s",
                projectName.c_str(), projectPath.c_str());
            if (m_onProjectCreatedCallback) {
                m_onProjectCreatedCallback(projectPath);
            }
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception creating project: " + std::string(e.what());
            ANI_LOG_ERROR("[ProjectSystem] %s", m_lastError.c_str());
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

            std::filesystem::create_directories(projPath / "data" / "capture");
            std::filesystem::create_directories(projPath / "data" / "quicksaves");

            auto fileSys = GetFilePathSystem();
            if (fileSys) {
                fileSys->SetPath("LastOpenProject", projectPath);
            }

            AddToRecentProjects(projectPath);

            ANI_LOG_INFO("[ProjectSystem] Loaded project: %s from %s",
                comp->settings.projectName.c_str(), projectPath.c_str());
            if (fileSys) {
                ANI_LOG_INFO("  - AssetsFolder: %s", fileSys->GetPath("AssetsFolder").c_str());
                ANI_LOG_INFO("  - OutputFolder: %s", fileSys->GetPath("OutputFolder").c_str());
            }

            // -----------------------------------------------------------------
            // 1) Plugins FIRST.
            //    - LoadStagingPlugins moves any staged DLLs into versioned slots.
            //    - SetProjectContext loads the project's plugin_state.json and
            //      enables the plugins that were enabled in that project. Enabling
            //      a plugin calls OnStudioInit, which registers its view types with
            //      the ViewManager.
            //    - PrepareProjectPlugins sweeps any remaining enabled-but-not-yet-
            //      enabled plugins (idempotent).
            // -----------------------------------------------------------------
            if (m_pluginManager) {
                m_pluginManager->LoadStagingPlugins(true);
                m_pluginManager->SetProjectContext(projectPath);
                m_pluginManager->PrepareProjectPlugins();
                ANI_LOG_INFO("[ProjectSystem] Plugins prepared before viewstate load");
            }

            // -----------------------------------------------------------------
            // 2) ViewState SECOND ? all view types (including plugin views) are
            //    now registered, so DeserializeViewLists can resolve them.
            // -----------------------------------------------------------------
            LoadViewState();

            if (m_viewManager) {
                auto allWorkspaces = m_viewManager->GetAllWorkspaces();
                if (allWorkspaces.empty()) {
                    ANI_LOG_INFO("[ProjectSystem] No workspaces found, creating default workspace");
                    GUI::WorkspaceID defaultWorkspace = m_viewManager->CreateView();
                    m_viewState.SetLastActiveWorkspace(defaultWorkspace);
                    m_viewManager->SetActiveWorkspace(defaultWorkspace);
                    ANI_LOG_INFO("[ProjectSystem] Created default workspace: %u", (unsigned)defaultWorkspace);
                }
                else {
                    ANI_LOG_INFO("[ProjectSystem] Loaded %zu workspaces", allWorkspaces.size());
                }
            }

            // -----------------------------------------------------------------
            // 3) Layout / window state / paths.
            // -----------------------------------------------------------------
            LoadImGuiLayout();
            LoadAndApplyProjectWindowState();

            UpdateProjectSpecificPaths();

            // 4) Pure notification callback last.
            if (m_onProjectLoadedCallback) {
                m_onProjectLoadedCallback(projectPath);
            }

            UpdateAutoSaveSettings();
            m_autoSaveTimer = 0.0f;

            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception loading project: " + std::string(e.what());
            ANI_LOG_ERROR("[ProjectSystem] %s", m_lastError.c_str());
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

            ANI_LOG_INFO("[ProjectSystem] Project saved: %s", comp->settings.projectName.c_str());
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception saving project: " + std::string(e.what());
            ANI_LOG_ERROR("[ProjectSystem] %s", m_lastError.c_str());
            return false;
        }
    }

    void ProjectSystem::CloseProject() {
        auto* comp = GetProjectComponent();
        if (!comp || !comp->isOpen) {
            ANI_LOG_INFO("[ProjectSystem] No project to close");
            return;
        }

        ANI_LOG_INFO("[ProjectSystem] CloseProject() called");
        ANI_LOG_INFO("[ProjectSystem] Closing project: %s", comp->settings.projectName.c_str());

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

        m_autoSaveTimer = 0.0f;

        ANI_LOG_INFO("[ProjectSystem] Project closed");

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
            ANI_LOG_INFO("[ProjectSystem] Applying project template: %s", template_.name.c_str());

            GUI::WorkspaceID currentWorkspace = m_viewState.GetLastActiveWorkspace();
            if (m_viewManager) {
                if (!template_.name.empty()) {
                    m_viewManager->SetWorkspaceName(currentWorkspace, template_.name);
                }

                for (const auto& viewTypeName : template_.defaultOpenViews) {
                    try {
                        ANI_LOG_INFO("[ProjectSystem] Adding view: %s to workspace: %u",
                            viewTypeName.c_str(), (unsigned)currentWorkspace);
                        GUI::ViewTypeID viewType = m_viewManager->GetViewType(viewTypeName);
                        m_viewManager->AddViewByType(currentWorkspace, viewType);
                        ANI_LOG_INFO("[ProjectSystem] Successfully added view: %s", viewTypeName.c_str());
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_ERROR("[ProjectSystem] Failed to add view %s: %s",
                            viewTypeName.c_str(), e.what());
                    }
                }
            }

            if (!template_.settings.empty()) {
                ANI_LOG_INFO("[ProjectSystem] Template has settings (not implemented yet)");
            }

            SaveProject();
            ANI_LOG_INFO("[ProjectSystem] Successfully applied template: %s", template_.name.c_str());
            return true;
        }
        catch (const std::exception& e) {
            m_lastError = "Exception applying project template: " + std::string(e.what());
            ANI_LOG_ERROR("[ProjectSystem] %s", m_lastError.c_str());
            return false;
        }
    }

    bool ProjectSystem::SaveViewState() {
        if (m_suppressViewStateSave) return false;
        if (!m_viewManager) return false;
        try {
            std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";
            ANI_LOG_INFO("[ProjectSystem] Saving ViewState with active workspace: %u",
                (unsigned)m_viewState.GetLastActiveWorkspace());
            return m_viewState.SaveViewManagerState(*m_viewManager, viewStatePath);
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ProjectSystem] Exception saving ViewState: %s", e.what());
            return false;
        }
    }

    bool ProjectSystem::LoadViewState() {
        if (!m_viewManager) return false;

        try {
            std::string viewStatePath = GetProjectDataPath() + "/viewstate.json";

            m_suppressCaptureApply = true;
            bool success = m_viewState.LoadViewManagerState(*m_viewManager, viewStatePath);
            m_suppressCaptureApply = false;

            if (success) {
                GUI::WorkspaceID lastActiveWorkspace = m_viewState.GetLastActiveWorkspace();
                auto allWorkspaces = m_viewManager->GetAllWorkspaces();

                if (std::find(allWorkspaces.begin(), allWorkspaces.end(), lastActiveWorkspace) == allWorkspaces.end()) {
                    if (!allWorkspaces.empty()) {
                        lastActiveWorkspace = allWorkspaces[0];
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        ANI_LOG_INFO("[ProjectSystem] Corrected active workspace to: %u",
                            (unsigned)lastActiveWorkspace);
                    }
                    else {
                        lastActiveWorkspace = m_viewManager->CreateView();
                        m_viewState.SetLastActiveWorkspace(lastActiveWorkspace);
                        ANI_LOG_INFO("[ProjectSystem] Created default workspace: %u",
                            (unsigned)lastActiveWorkspace);
                    }
                }

                m_viewManager->SetActiveWorkspace(lastActiveWorkspace);
                ANI_LOG_INFO("[ProjectSystem] Loaded ViewState with active workspace: %u",
                    (unsigned)lastActiveWorkspace);

                if (m_onViewStateLoadedCallback) {
                    m_onViewStateLoadedCallback(lastActiveWorkspace);
                }
            }

            return success;
        }
        catch (const std::exception& e) {
            m_suppressCaptureApply = false;
            ANI_LOG_ERROR("[ProjectSystem] Exception loading ViewState: %s", e.what());
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
            ANI_LOG_ERROR("[ProjectSystem] Exception saving ImGui layout: %s", e.what());
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
            ANI_LOG_ERROR("[ProjectSystem] Exception loading ImGui layout: %s", e.what());
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
            ANI_LOG_ERROR("[ProjectSystem] Exception saving window state: %s", e.what());
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

                ANI_LOG_INFO("[ProjectSystem] Applied project window state");
                return true;
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[ProjectSystem] Exception loading window state: %s", e.what());
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

        ANI_LOG_INFO("[ProjectSystem] Updated project-specific paths:");
        ANI_LOG_INFO("  - AssetsFolder: %s", assetsPath.c_str());
        ANI_LOG_INFO("  - OutputFolder: %s", outputPath.c_str());
        ANI_LOG_INFO("  - ProjectDataPath: %s", dataPath.c_str());
    }

    void ProjectSystem::ClearProjectSpecificPaths() {
        auto fileSys = GetFilePathSystem();
        if (fileSys) {
            fileSys->SetPath("AssetsFolder", "");
            fileSys->SetPath("OutputFolder", "");
            fileSys->SetPath("ProjectDataPath", "");
        }

        ANI_LOG_INFO("[ProjectSystem] Cleared project-specific paths");
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
        ANI_LOG_INFO("[ProjectSystem] ShouldShowStartup check:");
        ANI_LOG_INFO("  - Project open: %s", IsProjectOpen() ? "YES" : "NO");

        if (IsProjectOpen()) {
            ANI_LOG_INFO("  - Project already open");
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
                ANI_LOG_INFO("  - loadLastProject setting: %s", loadLastProject ? "YES" : "NO");
            }
        }

        if (!loadLastProject) {
            ANI_LOG_INFO("  - loadLastProject is disabled, showing startup");
            return true;
        }

        if (!lastProjectPath.empty() && std::filesystem::exists(lastProjectPath)) {
            ANI_LOG_INFO("  - Has last opened project: %s", lastProjectPath.c_str());
            const_cast<ProjectSystem*>(this)->LoadProject(lastProjectPath);
            return false;
        }

        ANI_LOG_INFO("  - No last opened project");
        return true;
    }

} // namespace ECS