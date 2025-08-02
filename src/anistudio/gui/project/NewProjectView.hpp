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