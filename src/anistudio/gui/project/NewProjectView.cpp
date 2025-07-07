#include "NewProjectView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace GUI {

	NewProjectView::NewProjectView(ECS::EntityManager& entityMgr, ANI::ProjectManager& projectMgr)
		: BaseView(entityMgr), m_projectManager(projectMgr) {
		viewName = "New Project";
	}

	void NewProjectView::Init() {
		LoadTemplates();
	}

	void NewProjectView::Update(const float deltaT) {
		// Nothing to update
	}

	void NewProjectView::Render() {
		if (ImGui::Begin(viewName.c_str())) {
			ImGui::Text("Create New Project");
			ImGui::Separator();

			// Project details - FIXED: Use char buffers
			ImGui::InputText("Project Name", m_projectNameBuffer, sizeof(m_projectNameBuffer));
			ImGui::InputText("Project Path", m_projectPathBuffer, sizeof(m_projectPathBuffer));
			ImGui::SameLine();
			if (ImGui::Button("Browse...")) {
				// TODO: File dialog
			}

			ImGui::Separator();

			// Template selection
			ShowTemplateSelector();

			ImGui::Separator();

			// Buttons
			if (ImGui::Button("Create Project")) {
				CreateProject();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel")) {
				// TODO: Close this view
			}
		}
		ImGui::End();
	}

	void NewProjectView::LoadTemplates() {
		m_templates.clear();

		std::string templatesDir = "../data/project_templates";

		try {
			if (!std::filesystem::exists(templatesDir)) {
				std::filesystem::create_directories(templatesDir);

				// Create default templates
				ProjectTemplate diffusionTemplate;
				diffusionTemplate.name = "AI Image Generation";
				diffusionTemplate.description = "Project for AI image generation with diffusion models";
				diffusionTemplate.category = "AI Generation";
				diffusionTemplate.defaultOpenViews = { "DiffusionView", "ImageView" };

				std::ofstream file(templatesDir + "/ai_generation.json");
				nlohmann::json j;
				j["name"] = diffusionTemplate.name;
				j["description"] = diffusionTemplate.description;
				j["category"] = diffusionTemplate.category;
				j["defaultOpenViews"] = diffusionTemplate.defaultOpenViews;
				file << j.dump(4);
				file.close();

				m_templates.push_back(diffusionTemplate);
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(templatesDir)) {
				if (entry.path().extension() == ".json") {
					std::ifstream file(entry.path());
					if (file.is_open()) {
						nlohmann::json j;
						file >> j;
						file.close();

						ProjectTemplate template_;
						if (j.contains("name")) template_.name = j["name"];
						if (j.contains("description")) template_.description = j["description"];
						if (j.contains("category")) template_.category = j["category"];
						if (j.contains("defaultOpenViews")) template_.defaultOpenViews = j["defaultOpenViews"];
						if (j.contains("settings")) template_.settings = j["settings"];

						m_templates.push_back(template_);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[NewProjectView] Failed to load templates: " << e.what() << std::endl;
		}
	}

	void NewProjectView::ShowTemplateSelector() {
		ImGui::Text("Choose Template:");

		// Blank project option
		bool isBlankSelected = (m_selectedTemplate == -1);
		if (ImGui::Selectable("Blank Project", isBlankSelected)) {
			m_selectedTemplate = -1;
		}
		if (isBlankSelected) {
			ImGui::Indent();
			ImGui::TextWrapped("Empty project with no views open");
			ImGui::Unindent();
		}

		ImGui::Separator();

		// Template options
		for (int i = 0; i < m_templates.size(); ++i) {
			const auto& template_ = m_templates[i];
			bool isSelected = (m_selectedTemplate == i);

			if (ImGui::Selectable(template_.name.c_str(), isSelected)) {
				m_selectedTemplate = i;
			}

			if (isSelected) {
				ImGui::Indent();
				ImGui::TextWrapped("Category: %s", template_.category.c_str());
				ImGui::TextWrapped("Description: %s", template_.description.c_str());

				if (!template_.defaultOpenViews.empty()) {
					ImGui::Text("Default Views:");
					for (const auto& viewType : template_.defaultOpenViews) {
						ImGui::BulletText("%s", viewType.c_str());
					}
				}
				ImGui::Unindent();
			}
		}
	}

	void NewProjectView::CreateProject() {
		std::string projectName = GetProjectName();
		std::string projectPath = GetProjectPath();

		if (projectName.empty() || projectPath.empty()) {
			return;
		}

		std::string fullPath = projectPath + "/" + projectName;

		if (m_projectManager.CreateNewProject(fullPath, projectName)) {
			// Apply template if selected
			if (m_selectedTemplate >= 0 && m_selectedTemplate < m_templates.size()) {
				const auto& template_ = m_templates[m_selectedTemplate];
				for (const auto& viewType : template_.defaultOpenViews) {
					m_projectManager.GetViewState().SetViewOpen(viewType, true);
				}
			}

			// TODO: Close this view
			std::cout << "[NewProjectView] Created project: " << projectName << std::endl;
		}
		else {
			std::cerr << "[NewProjectView] Failed to create project: " << m_projectManager.GetLastError() << std::endl;
		}
	}

} // namespace GUI