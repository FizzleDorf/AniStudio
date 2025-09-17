#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>
#include <imgui.h>

namespace ANI { class ProjectManager; }

namespace GUI {

	// Simple popup state structure
	struct ProjectPopupState {
		bool showNewProjectPopup = false;
		bool showLoadProjectPopup = false;

		// Project creation data
		char projectNameBuffer[256] = { 0 };
		char projectPathBuffer[512] = { 0 };
		std::vector<ProjectTemplate> templates;
		int selectedTemplate = -1;

		// Recent projects cache
		std::vector<std::string> recentProjects;

		ProjectPopupState() = default;

		void InitializeBuffers(ANI::ProjectManager& projectMgr);
		void LoadTemplates();
		void RefreshRecentProjects(ANI::ProjectManager& projectMgr);

		// Helper method for generating default project names
		std::string GenerateDefaultProjectName(ANI::ProjectManager& projectMgr) const;
	};

	// Static popup rendering functions that can be used anywhere
	namespace ProjectPopups {

		// Call these from any Render() function
		void RenderNewProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
		void RenderLoadProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr);

		// Helper functions
		void ShowTemplateSelector(ProjectPopupState& state);
		void ShowRecentProjects(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
		void CreateProject(ProjectPopupState& state, ANI::ProjectManager& projectMgr);

	} // namespace ProjectPopups

} // namespace GUI