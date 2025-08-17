#include "ProjectManagerView.hpp"
#include "ProjectManager.hpp"
#include "FilePaths.hpp"
#include <imgui.h>
#include <ImGuiFileDialog.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace GUI {

	ProjectManagerView::ProjectManagerView(ANI::ProjectManager& projectMgr)
		: m_projectManager(projectMgr), showNewProjectPopup(false), showLoadProjectPopup(false) {

		// Initialize project creation buffers with default paths from FilePaths
		memset(m_projectNameBuffer, 0, sizeof(m_projectNameBuffer));
		memset(m_projectPathBuffer, 0, sizeof(m_projectPathBuffer));

		// Set default project path from FilePaths utility
		std::string defaultPath = Utils::FilePaths::defaultProjectPath;
		if (!defaultPath.empty()) {
			strncpy_s(m_projectPathBuffer, defaultPath.c_str(), sizeof(m_projectPathBuffer) - 1);
		}
	}

	void ProjectManagerView::Init() {
		std::cout << "[ProjectManagerView] Startup view initialized" << std::endl;
		LoadTemplates();
	}

	void ProjectManagerView::Update(const float deltaT) {
		// Close this view if a project is now open
		if (m_projectManager.IsProjectOpen()) {
			// View will be hidden automatically when project opens
			// Reset popup flags when closing
			showNewProjectPopup = false;
			showLoadProjectPopup = false;
		}
	}

	void ProjectManagerView::Render() {
		// Don't render if project is open
		if (m_projectManager.IsProjectOpen()) {
			return;
		}

		// Center the window like a normal window
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

		// Normal window flags
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar;

		bool isOpen = true;
		if (ImGui::Begin("Welcome to AniStudio##StartupWindow", &isOpen, flags)) {

			ImGui::Text("Welcome to AniStudio");
			ImGui::Text("Media Creation & AI Generation Tool");
			ImGui::Separator();

			ImGui::Text("Recent Projects:");
			auto recentProjects = m_projectManager.GetRecentProjects();

			if (ImGui::BeginChild("RecentProjectsList", ImVec2(0, 180), true)) {
				if (recentProjects.empty()) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
					ImGui::Text("No recent projects found");
					ImGui::Text("Create a new project to get started!");
					ImGui::PopStyleColor();
				}
				else {
					for (const auto& projectPath : recentProjects) {
						std::filesystem::path path(projectPath);
						std::string displayName = path.filename().string();

						if (ImGui::Selectable(displayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
							if (ImGui::IsMouseDoubleClicked(0)) {
								if (m_projectManager.LoadProject(projectPath)) {
									std::cout << "[ProjectManagerView] Loaded project: " << projectPath << std::endl;
								}
								else {
									std::cerr << "[ProjectManagerView] Failed to load project: " << m_projectManager.GetLastError() << std::endl;
								}
							}
						}

						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Path: %s\nDouble-click to open", projectPath.c_str());
						}
					}
				}
			}
			ImGui::EndChild();

			ImGui::Separator();

			// Action buttons
			float buttonWidth = 150.0f;
			float buttonHeight = 40.0f;
			float spacing = 15.0f;
			float totalWidth = buttonWidth * 3 + spacing * 2;
			float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

			ImGui::SetCursorPosX(startX);
			if (ImGui::Button("New Project", ImVec2(buttonWidth, buttonHeight))) {
				showNewProjectPopup = true;
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Open Project", ImVec2(buttonWidth, buttonHeight))) {
				showLoadProjectPopup = true;
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Exit", ImVec2(buttonWidth, buttonHeight))) {
				exit(0);
			}

		}
		ImGui::End();

		// Handle close or ESC
		if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			// View will be hidden automatically when project opens
		}

		// Handle popups like your settings example
		RenderNewProjectPopup();
		RenderLoadProjectPopup();
	}

	void ProjectManagerView::RenderNewProjectPopup() {
		// New Project Popup
		if (showNewProjectPopup) {
			ImGui::OpenPopup("Create New Project");
		}

		if (ImGui::BeginPopupModal("Create New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Create New Project");
			ImGui::Separator();

			ImGui::Text("Project Name:");
			ImGui::InputText("##ProjectName", m_projectNameBuffer, sizeof(m_projectNameBuffer));

			ImGui::Text("Project Path:");
			ImGui::InputText("##ProjectPath", m_projectPathBuffer, sizeof(m_projectPathBuffer));
			ImGui::SameLine();
			if (ImGui::Button("Browse...")) {
				// FIXED: Use proper FileDialogConfig instead of passing string directly
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::defaultProjectPath;
				config.flags = ImGuiFileDialogFlags_Modal;
				ImGuiFileDialog::Instance()->OpenDialog("ChooseProjectPath", "Choose Project Directory",
					nullptr, config);
			}

			// FIXED: Handle file dialog result
			if (ImGuiFileDialog::Instance()->Display("ChooseProjectPath")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
					strncpy_s(m_projectPathBuffer, selectedPath.c_str(), sizeof(m_projectPathBuffer) - 1);
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::Separator();

			// Template selection
			ShowTemplateSelector();

			ImGui::Separator();

			float buttonWidth = 120.0f;
			float spacing = 10.0f;
			float totalButtonWidth = buttonWidth * 2 + spacing;
			float startX = (ImGui::GetContentRegionAvail().x - totalButtonWidth) * 0.5f;

			ImGui::SetCursorPosX(startX);
			if (ImGui::Button("Create", ImVec2(buttonWidth, 30))) {
				CreateProject();
				showNewProjectPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
				showNewProjectPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ProjectManagerView::RenderLoadProjectPopup() {
		// Load Project Popup
		if (showLoadProjectPopup) {
			ImGui::OpenPopup("Load Project");
		}

		if (ImGui::BeginPopupModal("Load Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Load Project");
			ImGui::Separator();

			ImGui::Text("Recent Projects:");
			if (ImGui::BeginChild("RecentProjectsPopup", ImVec2(400, 200), true)) {
				auto recentProjects = m_projectManager.GetRecentProjects();
				if (recentProjects.empty()) {
					ImGui::TextDisabled("No recent projects found");
				}
				else {
					for (const auto& projectPath : recentProjects) {
						std::filesystem::path path(projectPath);
						std::string displayName = path.filename().string();

						if (ImGui::Selectable(displayName.c_str())) {
							if (m_projectManager.LoadProject(projectPath)) {
								std::cout << "[ProjectManagerView] Loaded project: " << projectPath << std::endl;
								showLoadProjectPopup = false;
								ImGui::CloseCurrentPopup();
							}
						}

						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("%s", projectPath.c_str());
						}
					}
				}
			}
			ImGui::EndChild();

			ImGui::Separator();

			float buttonWidth = 120.0f;
			float spacing = 10.0f;
			float totalButtonWidth = buttonWidth * 2 + spacing;
			float startX = (ImGui::GetContentRegionAvail().x - totalButtonWidth) * 0.5f;

			ImGui::SetCursorPosX(startX);
			if (ImGui::Button("Browse...", ImVec2(buttonWidth, 30))) {
				// FIXED: Use proper FileDialogConfig for project file selection
				IGFD::FileDialogConfig config;
				config.path = Utils::FilePaths::defaultProjectPath;
				config.flags = ImGuiFileDialogFlags_Modal;
				ImGuiFileDialog::Instance()->OpenDialog("ChooseProjectFile", "Choose Project File",
					".json", config);
			}

			// FIXED: Handle file dialog result
			if (ImGuiFileDialog::Instance()->Display("ChooseProjectFile")) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
					if (m_projectManager.LoadProject(selectedFile)) {
						std::cout << "[ProjectManagerView] Loaded project: " << selectedFile << std::endl;
						showLoadProjectPopup = false;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
				showLoadProjectPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void ProjectManagerView::LoadTemplates() {
		m_templates.clear();

		std::string templatesDir = Utils::FilePaths::dataPath + "/project_templates";

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
			std::cerr << "[ProjectManagerView] Failed to load templates: " << e.what() << std::endl;
		}
	}

	void ProjectManagerView::ShowTemplateSelector() {
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

	void ProjectManagerView::CreateProject() {
		std::string projectName = std::string(m_projectNameBuffer);
		std::string projectPath = std::string(m_projectPathBuffer);

		if (projectName.empty() || projectPath.empty()) {
			std::cerr << "[ProjectManagerView] Project name and path cannot be empty" << std::endl;
			return;
		}

		std::string fullPath = projectPath + "/" + projectName;

		if (m_projectManager.CreateNewProject(fullPath, projectName)) {
			// TODO: Workspace creation will be handled by the ViewManager
			// For now, the ViewManager will create default workspaces automatically
			if (m_selectedTemplate >= 0 && m_selectedTemplate < m_templates.size()) {
				const auto& template_ = m_templates[m_selectedTemplate];
				std::cout << "[ProjectManagerView] Will create workspace from template: " << template_.name << std::endl;
				// TODO: Use events to create workspace with template views
			}
			else {
				std::cout << "[ProjectManagerView] Will create blank workspace" << std::endl;
			}

			// View will close automatically in Update() when project is created
			std::cout << "[ProjectManagerView] Created project: " << projectName << std::endl;
		}
		else {
			std::cerr << "[ProjectManagerView] Failed to create project: " << m_projectManager.GetLastError() << std::endl;
		}
	}

	// Public methods for MenuBar to trigger popups
	void ProjectManagerView::ShowNewProjectDialog() {
		showNewProjectPopup = true;
	}

	void ProjectManagerView::ShowLoadProjectDialog() {
		showLoadProjectPopup = true;
	}

} // namespace GUI