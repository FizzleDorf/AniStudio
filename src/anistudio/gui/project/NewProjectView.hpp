#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>

namespace ANI { class ProjectManager; }

namespace GUI {

	// Standalone new project modal - not derived from BaseView
	class NewProjectView {
	public:
		NewProjectView(ANI::ProjectManager& projectMgr);

		void Init();
		void Update(const float deltaT);
		void Render();

		// Control visibility
		void SetVisible(bool visible) { m_showPopup = visible; }

	private:
		ANI::ProjectManager& m_projectManager;

		std::vector<ProjectTemplate> m_templates;
		int m_selectedTemplate = -1;

		// Project input buffers
		char m_projectNameBuffer[256];
		char m_projectPathBuffer[512];

		// Simple popup state
		bool m_showPopup = false;

		void LoadTemplates();
		void ShowTemplateSelector();
		void CreateProject();

		// Helper functions to convert between std::string and char buffers
		std::string GetProjectName() const { return std::string(m_projectNameBuffer); }
		std::string GetProjectPath() const { return std::string(m_projectPathBuffer); }
	};

} // namespace GUI