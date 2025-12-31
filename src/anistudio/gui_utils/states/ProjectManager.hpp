#pragma once
#include "ViewState.hpp"
#include "FilePathService.hpp"
#include "ProjectTemplate.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>

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

		// Window handle management for window state saving/loading
		void SetWindowHandle(void* windowHandle);

		// Startup detection - now uses FilePaths utility
		bool ShouldShowStartup() const;

		// Project lifecycle
		bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
		bool LoadProject(const std::string& projectPath);
		bool SaveProject();
		void CloseProject();

		// Project template application
		bool ApplyProjectTemplate(const GUI::ProjectTemplate& template_);

		// Project validation
		bool IsProjectNameTaken(const std::string& projectName, const std::string& excludePath = "") const;

		// Project state
		bool IsProjectOpen() const { return m_isProjectOpen; }
		const std::string& GetCurrentProjectPath() const { return m_currentProjectPath; }
		const std::string& GetCurrentProjectName() const { return m_projectSettings.projectName; }

		// Project data path access (for internal use)
		std::string GetProjectDataPath() const;

		// CRITICAL: ViewState access for workspace management
		GUI::ViewState& GetViewState() { return m_viewState; }
		const GUI::ViewState& GetViewState() const { return m_viewState; }

		// Convenience methods for workspace management
		void SetLastActiveWorkspace(GUI::WorkspaceID workspaceID) {
			m_viewState.SetLastActiveWorkspace(workspaceID);
		}

		GUI::WorkspaceID GetLastActiveWorkspace() const {
			return m_viewState.GetLastActiveWorkspace();
		}

		// Recent projects - now using FilePaths utility for storage
		std::vector<std::string> GetRecentProjects() const;
		void AddToRecentProjects(const std::string& projectPath);

		// Path management - delegates to FilePaths utility
		void SetDefaultProjectPath(const std::string& path);
		std::string GetDefaultProjectPath() const;
		void SetAssetsFolder(const std::string& path);
		std::string GetAssetsFolder() const;

		// Output folder management
		void SetOutputFolder(const std::string& path);
		std::string GetOutputFolder() const;

		// Application-level path initialization
		static void InitializeApplicationPaths();

		// Error handling
		const std::string& GetLastError() const { return m_lastError; }

		// Project event callbacks
		void SetProjectLoadedCallback(std::function<void(const std::string&)> callback) {
			m_onProjectLoadedCallback = callback;
		}

		void SetProjectCreatedCallback(std::function<void(const std::string&)> callback) {
			m_onProjectCreatedCallback = callback;
		}

		void SetProjectClosedCallback(std::function<void()> callback) {
			m_onProjectClosedCallback = callback;
		}

		// Callback for when ViewState is loaded with active workspace
		void SetViewStateLoadedCallback(std::function<void(GUI::WorkspaceID)> callback) {
			m_onViewStateLoadedCallback = callback;
		}

	private:
		GUI::ViewManager& m_viewManager;
		ECS::EntityManager& m_entityManager;

		bool m_isProjectOpen = false;
		std::string m_currentProjectPath;
		ProjectSettings m_projectSettings;
		GUI::ViewState m_viewState;
		std::string m_lastError;

		// Window handle for window state management
		void* m_windowHandle = nullptr;

		// Project event callbacks
		std::function<void(const std::string&)> m_onProjectLoadedCallback;
		std::function<void(const std::string&)> m_onProjectCreatedCallback;
		std::function<void()> m_onProjectClosedCallback;
		std::function<void(GUI::WorkspaceID)> m_onViewStateLoadedCallback;

		// File operations
		bool SaveViewState();
		bool LoadViewState();
		bool SaveImGuiLayout();
		bool LoadImGuiLayout();

		// Window state management
		bool SaveProjectWindowState();
		bool LoadAndApplyProjectWindowState();

		// Project-specific path management
		void UpdateProjectSpecificPaths();
		void ClearProjectSpecificPaths();

		// Path utilities
		std::string GetProjectAssetsPath() const;
		std::string GetProjectOutputPath() const;
		std::string GetProjectWindowStatePath() const;
	};

} // namespace ANI