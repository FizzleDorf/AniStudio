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
#include "ViewManager.hpp"
#include "ECS.h"
#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>
#include <memory>

namespace ANI {

	struct ProjectSettings {
		std::string projectName = "Untitled Project";
		std::string projectVersion = "1.0.0";
		std::string projectDescription;
		std::string createdBy;
		std::string createdDate;
		std::string lastModified;

		// Project-specific model paths
		std::string projectModelRoot;
		std::string projectCheckpointDir;
		std::string projectVaeDir;
		std::string projectLoraDir;
		std::string projectControlnetDir;
		std::string projectUpscaleDir;

		nlohmann::json Serialize() const;
		void Deserialize(const nlohmann::json& j);
	};

	class ProjectManager {
	public:
		ProjectManager(GUI::ViewManager& viewMgr, ECS::EntityManager& entityMgr);
		~ProjectManager();

		// Project lifecycle
		bool CreateNewProject(const std::string& projectPath, const std::string& projectName);
		bool LoadProject(const std::string& projectPath);
		bool SaveProject();
		bool SaveProjectAs(const std::string& newPath);
		void CloseProject();

		// Project state
		bool IsProjectOpen() const { return m_isProjectOpen; }
		const std::string& GetCurrentProjectPath() const { return m_currentProjectPath; }
		const std::string& GetCurrentProjectName() const { return m_projectSettings.projectName; }
		const ProjectSettings& GetProjectSettings() const { return m_projectSettings; }
		ProjectSettings& GetProjectSettings() { return m_projectSettings; }

		// ViewState management
		bool SaveViewState();
		bool LoadViewState();
		bool HasSavedViewState() const;

		// Auto-save functionality
		void SetAutoSave(bool enabled) { m_autoSaveEnabled = enabled; }
		bool IsAutoSaveEnabled() const { return m_autoSaveEnabled; }
		void SetAutoSaveInterval(float seconds) { m_autoSaveInterval = seconds; }
		void Update(float deltaTime);

		// Recent projects
		std::vector<std::string> GetRecentProjects() const;
		void AddToRecentProjects(const std::string& projectPath);

		// Project templates
		bool CreateProjectFromTemplate(const std::string& templateName, const std::string& projectPath, const std::string& projectName);
		std::vector<std::string> GetAvailableTemplates() const;

		// Error handling
		const std::string& GetLastError() const { return m_lastError; }

	private:
		// References to core systems
		GUI::ViewManager& m_viewManager;
		ECS::EntityManager& m_entityManager;

		// Project state
		bool m_isProjectOpen = false;
		std::string m_currentProjectPath;
		ProjectSettings m_projectSettings;
		GUI::ViewState m_viewState;

		// Auto-save
		bool m_autoSaveEnabled = true;
		float m_autoSaveInterval = 300.0f; // 5 minutes
		float m_autoSaveTimer = 0.0f;
		bool m_hasUnsavedChanges = false;

		// Error handling
		std::string m_lastError;

		// File paths
		std::string GetProjectFilePath() const;
		std::string GetViewStateFilePath() const;
		std::string GetSettingsFilePath() const;
		std::string GetAssetsDirectoryPath() const;
		std::string GetScriptsDirectoryPath() const;
		std::string GetModelsDirectoryPath() const;

		// Internal methods
		bool CreateProjectDirectory(const std::string& projectPath);
		bool SetupProjectStructure(const std::string& projectPath);
		void UpdateFilePaths();
		void MarkAsModified();
		bool ValidateProjectPath(const std::string& path) const;

		// Serialization helpers
		nlohmann::json SerializeProjectData() const;
		bool DeserializeProjectData(const nlohmann::json& j);

		// Recent projects management
		void LoadRecentProjects();
		void SaveRecentProjects();
		std::vector<std::string> m_recentProjects;
		static constexpr size_t MAX_RECENT_PROJECTS = 10;
	};

} // namespace ANI