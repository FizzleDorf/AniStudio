#include "NewProjectView.hpp"
#include "ProjectManager.hpp"
#include "Events.hpp"
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace GUI {

	NewProjectView::NewProjectView(ANI::ProjectManager& projectMgr)
		: m_projectManager(projectMgr) {

		// Initialize buffers
		memset(m_projectNameBuffer, 0, sizeof(m_projectNameBuffer));
		memset(m_projectPathBuffer, 0, sizeof(m_projectPathBuffer));

		// Set default project path
		std::string defaultPath = m_projectManager.GetDefaultProjectPath();
		if (!defaultPath.empty()) {
			strncpy_s(m_projectPathBuffer, defaultPath.c_str(), sizeof(m_projectPathBuffer) - 1);
		}
	}

	void NewProjectView::Init() {
		LoadTemplates();
	}

	void NewProjectView::Update(const float deltaT) {
		// Close popup if a project was created/loaded
		if (m_projectManager.IsProjectOpen()) {
			m_showPopup = false;
		}
	}

	void NewProjectView::Render() {
		if (!m_showPopup) {
			return;
		}

		if (m_showPopup) {
			ImGui::OpenPopup("Create New Project##NewProjectPopup");
		}

		// Center and make it a proper modal popup
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(550, 450), ImGuiCond_Appearing);

		// Modal popup flags
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_Modal;

		bool isOpen = true;
		if (ImGui::BeginPopupModal("Create New Project##NewProjectPopup", &isOpen, flags)) {
			ImGui::Text("Create New Project");
			ImGui::Separator();

			// Project details
			ImGui::Text("Project Name:");
			ImGui::InputText("##ProjectName", m_projectNameBuffer, sizeof(m_projectNameBuffer));

			ImGui::Text("Project Path:");
			ImGui::InputText("##ProjectPath", m_projectPathBuffer, sizeof(m_projectPathBuffer));
			ImGui::SameLine();
			if (ImGui::Button("Browse...##PathBrowse")) {
				std::cout << "[NewProjectView] Browse button clicked" << std::endl;
			}

			ImGui::Separator();

			// Template selection
			ShowTemplateSelector();

			ImGui::Separator();

			// Buttons
			float buttonWidth = 120.0f;
			float spacing = 10.0f;
			float totalWidth = buttonWidth * 2 + spacing;
			float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

			ImGui::SetCursorPosX(startX);
			if (ImGui::Button("Create Project", ImVec2(buttonWidth, 30))) {
				CreateProject();
				m_showPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
				m_showPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Handle close button or ESC key
		if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			m_showPopup = false;
		}
	}

	void NewProjectView::LoadTemplates() {
		m_templates.clear();

		std::string templatesDir = "../data/project_templates";

		try {
			if (!std::filesystem::exists(templatesDir)) {
				std::filesystem::create_directories(templatesDir);

				// Create default template
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

		if (ImGui::BeginChild("TemplateList", ImVec2(0, 200), true)) {
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

			if (!m_templates.empty()) {
				ImGui::Separator();
			}

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
		ImGui::EndChild();
	}

	void NewProjectView::CreateProject() {
		std::string projectName = GetProjectName();
		std::string projectPath = GetProjectPath();

		if (projectName.empty() || projectPath.empty()) {
			std::cerr << "[NewProjectView] Project name and path cannot be empty" << std::endl;
			return;
		}

		std::string fullPath = projectPath + "/" + projectName;

		if (m_projectManager.CreateNewProject(fullPath, projectName)) {
			// Use events to create workspace based on selected template
			auto& events = ANI::Events::Ref();

			if (m_selectedTemplate >= 0 && m_selectedTemplate < m_templates.size()) {
				// Use selected template - create workspace and add default views
				const auto& template_ = m_templates[m_selectedTemplate];

				events.RequestCreateWorkspace(template_.name);

				// Queue events to add the default views after workspace is created
				// Note: This assumes the workspace will be created with the next available ID
				// In a real implementation, you might need a callback system or queue the view adds
				for (const auto& viewTypeName : template_.defaultOpenViews) {
					// This is a simplified approach - in practice you'd need to know the workspace ID
					std::cout << "[NewProjectView] Will add view: " << viewTypeName << " to new workspace" << std::endl;
				}

				std::cout << "[NewProjectView] Created workspace from template: " << template_.name << std::endl;
			}
			else {
				// Blank project - create empty workspace
				events.RequestCreateWorkspace("Main");
				std::cout << "[NewProjectView] Created blank workspace" << std::endl;
			}

			std::cout << "[NewProjectView] Created project: " << projectName << std::endl;
		}
		else {
			std::cerr << "[NewProjectView] Failed to create project: " << m_projectManager.GetLastError() << std::endl;
		}
	}

} // namespace GUI