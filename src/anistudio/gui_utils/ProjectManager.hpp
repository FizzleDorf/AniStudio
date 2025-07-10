/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 */

#pragma once
#include "ViewState.hpp"
#include "FilePaths.hpp"
#include "OpenGLWrapper.hpp"
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

		// Window handle management
		void SetWindowHandle(void* windowHandle);

		// Startup detection - now uses FilePaths utility
		bool ShouldShowStartup() const;

		// Project lifecycle
		bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
		bool LoadProject(const std::string& projectPath);
		bool SaveProject();
		void CloseProject();

		// Project state
		bool IsProjectOpen() const { return m_isProjectOpen; }
		const std::string& GetCurrentProjectPath() const { return m_currentProjectPath; }
		const std::string& GetCurrentProjectName() const { return m_projectSettings.projectName; }

		// ViewState
		GUI::ViewState& GetViewState() { return m_viewState; }
		const GUI::ViewState& GetViewState() const { return m_viewState; }

		// Recent projects - now using FilePaths utility for storage
		std::vector<std::string> GetRecentProjects() const;
		void AddToRecentProjects(const std::string& projectPath);

		// Path management - delegates to FilePaths utility
		void SetDefaultProjectPath(const std::string& path);
		std::string GetDefaultProjectPath() const;
		void SetAssetsFolder(const std::string& path);
		std::string GetAssetsFolder() const;

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

	private:
		GUI::ViewManager& m_viewManager;
		ECS::EntityManager& m_entityManager;

		bool m_isProjectOpen = false;
		std::string m_currentProjectPath;
		ProjectSettings m_projectSettings;
		GUI::ViewState m_viewState;
		std::string m_lastError;

		// Window handle for direct GLFW access
		void* m_windowHandle;

		// Project event callbacks
		std::function<void(const std::string&)> m_onProjectLoadedCallback;
		std::function<void(const std::string&)> m_onProjectCreatedCallback;
		std::function<void()> m_onProjectClosedCallback;

		// File operations
		bool SaveViewState();
		bool LoadViewState();
		bool SaveImGuiLayout();
		bool LoadImGuiLayout();

		// Window state operations - now directly work with GLFW
		bool SaveProjectWindowState();
		bool LoadAndApplyProjectWindowState();
		std::string GetProjectWindowStatePath() const;

		// Project-specific path management
		void UpdateProjectSpecificPaths();
		void ClearProjectSpecificPaths();

		// Path utilities
		std::string GetProjectDataPath() const;
		std::string GetProjectAssetsPath() const;
		std::string GetProjectOutputPath() const;
	};

} // namespace ANI