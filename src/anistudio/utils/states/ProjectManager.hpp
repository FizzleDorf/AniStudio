#pragma once
#include "ViewState.hpp"
#include "FilePathSystem.hpp"
#include "ProjectTemplate.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace GUI { class ViewManager; }
namespace ECS { class EntityManager; }

namespace ANI {

    struct ProjectSettings {
        std::string projectName = "Untitled Project";
        std::string projectVersion = "1.0.0";
        std::string projectDescription;
        std::string createdBy;
        std::string createdDate;
        std::string lastModified;

        nlohmann::json Serialize() const;
        void Deserialize(const nlohmann::json& j);
    };

    class ProjectManager {
    public:
        ProjectManager(GUI::ViewManager& viewMgr, ECS::EntityManager& entityMgr);
        ~ProjectManager();

        void SetWindowHandle(void* windowHandle);

        bool ShouldShowStartup() const;

        bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
        bool LoadProject(const std::string& projectPath);
        bool SaveProject();
        void CloseProject();

        bool ApplyProjectTemplate(const GUI::ProjectTemplate& template_);

        bool IsProjectNameTaken(const std::string& projectName, const std::string& excludePath = "") const;

        bool IsProjectOpen() const { return m_isProjectOpen; }
        const std::string& GetCurrentProjectPath() const { return m_currentProjectPath; }
        const std::string& GetCurrentProjectName() const { return m_projectSettings.projectName; }

        std::string GetProjectDataPath() const;

        GUI::ViewState& GetViewState() { return m_viewState; }
        const GUI::ViewState& GetViewState() const { return m_viewState; }

        void SetLastActiveWorkspace(GUI::WorkspaceID workspaceID) {
            m_viewState.SetLastActiveWorkspace(workspaceID);
        }

        GUI::WorkspaceID GetLastActiveWorkspace() const {
            return m_viewState.GetLastActiveWorkspace();
        }

        std::vector<std::string> GetRecentProjects() const;
        void AddToRecentProjects(const std::string& projectPath);

        void SetDefaultProjectPath(const std::string& path);
        std::string GetDefaultProjectPath() const;
        void SetAssetsFolder(const std::string& path);
        std::string GetAssetsFolder() const;

        void SetOutputFolder(const std::string& path);
        std::string GetOutputFolder() const;

        const std::string& GetLastError() const { return m_lastError; }

        void SetProjectLoadedCallback(std::function<void(const std::string&)> callback) {
            m_onProjectLoadedCallback = callback;
        }

        void SetProjectCreatedCallback(std::function<void(const std::string&)> callback) {
            m_onProjectCreatedCallback = callback;
        }

        void SetProjectClosedCallback(std::function<void()> callback) {
            m_onProjectClosedCallback = callback;
        }

        void SetViewStateLoadedCallback(std::function<void(GUI::WorkspaceID)> callback) {
            m_onViewStateLoadedCallback = callback;
        }

        // Return a shared_ptr to the FilePathSystem
        std::shared_ptr<ECS::FilePathSystem> GetFilePathSystem() const;

    private:
        GUI::ViewManager& m_viewManager;
        ECS::EntityManager& m_entityManager;

        bool m_isProjectOpen = false;
        std::string m_currentProjectPath;
        ProjectSettings m_projectSettings;
        GUI::ViewState m_viewState;
        std::string m_lastError;

        void* m_windowHandle = nullptr;

        std::function<void(const std::string&)> m_onProjectLoadedCallback;
        std::function<void(const std::string&)> m_onProjectCreatedCallback;
        std::function<void()> m_onProjectClosedCallback;
        std::function<void(GUI::WorkspaceID)> m_onViewStateLoadedCallback;

        bool SaveViewState();
        bool LoadViewState();
        bool SaveImGuiLayout();
        bool LoadImGuiLayout();

        bool SaveProjectWindowState();
        bool LoadAndApplyProjectWindowState();

        void UpdateProjectSpecificPaths();
        void ClearProjectSpecificPaths();

        std::string GetProjectAssetsPath() const;
        std::string GetProjectOutputPath() const;
        std::string GetProjectWindowStatePath() const;
    };

} // namespace ANI