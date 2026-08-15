#pragma once
#include "BaseSystem.hpp"
#include "ProjectComponent.hpp"
#include "ViewState.hpp"
#include "Types.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace GUI {
    class ViewManager;
    struct ProjectTemplate;
}
namespace ECS { class FilePathSystem; }

namespace Plugins {
    class StudioPluginManager;
}

namespace ANI {

    class ProjectSystem : public ECS::BaseSystem {
    public:
        explicit ProjectSystem(ECS::EntityManager& mgr);
        virtual ~ProjectSystem() = default;

        virtual void Start() override;
        virtual void Update(float deltaT) override {}
        virtual void Destroy() override;

        void SetWindowHandle(void* windowHandle);
        void SetViewManager(GUI::ViewManager* viewManager);
        void SetPluginManager(Plugins::StudioPluginManager* pluginManager);

        bool ShouldShowStartup() const;
        bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
        bool LoadProject(const std::string& projectPath);
        bool SaveProject();
        void CloseProject();

        bool ApplyProjectTemplate(const GUI::ProjectTemplate& template_);

        bool IsProjectNameTaken(const std::string& projectName, const std::string& excludePath = "") const;

        bool IsProjectOpen() const;
        const std::string& GetCurrentProjectPath() const;
        const std::string& GetCurrentProjectName() const;
        std::string GetProjectDataPath() const;

        GUI::ViewState& GetViewState();
        const GUI::ViewState& GetViewState() const;

        void SetLastActiveWorkspace(GUI::WorkspaceID workspaceID);
        GUI::WorkspaceID GetLastActiveWorkspace() const;

        std::vector<std::string> GetRecentProjects() const;

        const std::string& GetLastError() const { return m_lastError; }

        void SetProjectLoadedCallback(std::function<void(const std::string&)> callback);
        void SetProjectCreatedCallback(std::function<void(const std::string&)> callback);
        void SetProjectClosedCallback(std::function<void()> callback);
        void SetViewStateLoadedCallback(std::function<void(GUI::WorkspaceID)> callback);

        ECS::EntityID GetProjectEntity() const { return m_projectEntity; }
        ProjectComponent* GetProjectComponent() const;

        std::shared_ptr<ECS::FilePathSystem> GetFilePathSystem() const;
        std::string GetDefaultProjectPath() const;
        void SetDefaultProjectPath(const std::string& path);

        bool SaveViewState();
        bool LoadViewState();

    private:
        ECS::EntityID m_projectEntity;
        ECS::ComponentTypeID m_componentTypeId;
        GUI::ViewManager* m_viewManager;
        Plugins::StudioPluginManager* m_pluginManager;
        void* m_windowHandle;
        std::string m_lastError;
        GUI::ViewState m_viewState;

        std::function<void(const std::string&)> m_onProjectLoadedCallback;
        std::function<void(const std::string&)> m_onProjectCreatedCallback;
        std::function<void()> m_onProjectClosedCallback;
        std::function<void(GUI::WorkspaceID)> m_onViewStateLoadedCallback;

        bool SaveImGuiLayout();
        bool LoadImGuiLayout();
        bool SaveProjectWindowState();
        bool LoadAndApplyProjectWindowState();

        void UpdateProjectSpecificPaths();
        void ClearProjectSpecificPaths();

        std::string GetProjectAssetsPath() const;
        std::string GetProjectOutputPath() const;
        std::string GetProjectWindowStatePath() const;

        void AddToRecentProjects(const std::string& projectPath);
        std::string GenerateDefaultProjectName() const;
    };

} // namespace ANI