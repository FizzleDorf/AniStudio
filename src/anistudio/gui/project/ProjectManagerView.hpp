#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>

namespace ANI { class ProjectManager; }

namespace GUI {

	class ProjectManagerView {
	public:
		ProjectManagerView(ANI::ProjectManager& projectMgr);

		void Init();
		void Update(const float deltaT);
		void Render();

		// Public methods for MenuBar to trigger popups
		void ShowNewProjectDialog();
		void ShowLoadProjectDialog();

	private:
		ANI::ProjectManager& m_projectManager;

		// Popup flags like your settings example
		bool showNewProjectPopup;
		bool showLoadProjectPopup;

		// Project creation data
		char m_projectNameBuffer[256];
		char m_projectPathBuffer[512];
		std::vector<ProjectTemplate> m_templates;
		int m_selectedTemplate = -1;

		// Private methods
		void RenderNewProjectPopup();
		void RenderLoadProjectPopup();
		void LoadTemplates();
		void ShowTemplateSelector();
		void CreateProject();
	};

} // namespace GUI