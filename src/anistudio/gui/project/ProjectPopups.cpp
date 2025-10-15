#include "ProjectPopups.hpp"
#include "ProjectManager.hpp"
#include "Events.hpp"
#include "FilePaths.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <set>
#include <ctime>

namespace GUI {

	void ProjectPopupState::InitializeBuffers(ANI::ProjectManager& projectMgr) {
		// Generate default project name
		std::string defaultProjectName = GenerateDefaultProjectName(projectMgr);

		// Initialize buffers
		memset(projectNameBuffer, 0, sizeof(projectNameBuffer));
		memset(projectPathBuffer, 0, sizeof(projectPathBuffer));

		// Set default project name
		strncpy(projectNameBuffer, defaultProjectName.c_str(), sizeof(projectNameBuffer) - 1);
		projectNameBuffer[sizeof(projectNameBuffer) - 1] = '\0'; // Ensure null termination

		// Set default project path
		std::string defaultPath = projectMgr.GetDefaultProjectPath();
		if (!defaultPath.empty()) {
			strncpy(projectPathBuffer, defaultPath.c_str(), sizeof(projectPathBuffer) - 1);
			projectPathBuffer[sizeof(projectPathBuffer) - 1] = '\0'; // Ensure null termination
		}
	}

	std::string ProjectPopupState::GenerateDefaultProjectName(ANI::ProjectManager& projectMgr) const {
		std::string baseName = "AniProject";
		std::string defaultPath = projectMgr.GetDefaultProjectPath();

		if (defaultPath.empty()) {
			return baseName + "1";
		}

		// Find the next available project name
		int counter = 1;
		std::string candidateName;

		do {
			candidateName = baseName + std::to_string(counter);
			counter++;

			// Prevent infinite loop
			if (counter > 9999) {
				candidateName = baseName + "_" + std::to_string(time(nullptr));
				break;
			}
		} while (projectMgr.IsProjectNameTaken(candidateName));

		return candidateName;
	}

	void ProjectPopupState::LoadTemplates() {
		templates.clear();

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

				templates.push_back(diffusionTemplate);
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

						templates.push_back(template_);
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[ProjectPopups] Failed to load templates: " << e.what() << std::endl;
		}
	}

	void ProjectPopupState::RefreshRecentProjects(ANI::ProjectManager& projectMgr) {
		recentProjects = projectMgr.GetRecentProjects();
	}

	namespace ProjectPopups {

		void RenderNewProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr) {
			if (state.showNewProjectPopup) {
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
				ImGui::InputText("##ProjectName", state.projectNameBuffer, sizeof(state.projectNameBuffer));

				ImGui::Text("Project Path:");
				ImGui::InputText("##ProjectPath", state.projectPathBuffer, sizeof(state.projectPathBuffer));
				ImGui::SameLine();
				if (ImGui::Button("Browse...##PathBrowse")) {
					std::cout << "[ProjectPopups] Browse button clicked" << std::endl;
					// TODO: Implement file dialog
				}

				ImGui::Separator();

				// Template selection
				ShowTemplateSelector(state);

				ImGui::Separator();

				// Buttons
				float buttonWidth = 120.0f;
				float spacing = 10.0f;
				float totalWidth = buttonWidth * 2 + spacing;
				float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

				ImGui::SetCursorPosX(startX);
				if (ImGui::Button("Create Project", ImVec2(buttonWidth, 30))) {
					CreateProject(state, projectMgr);
					state.showNewProjectPopup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SameLine(0, spacing);
				if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
					state.showNewProjectPopup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			// Handle close button or ESC key
			if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				state.showNewProjectPopup = false;
			}
		}

		void RenderLoadProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr) {
			if (state.showLoadProjectPopup) {
				ImGui::OpenPopup("Load Project##LoadProjectPopup");
			}

			// Center and make it a proper modal popup
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);

			// Modal popup flags
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
				ImGuiWindowFlags_Modal;

			bool isOpen = true;
			if (ImGui::BeginPopupModal("Load Project##LoadProjectPopup", &isOpen, flags)) {
				ImGui::Text("Load Project");
				ImGui::Separator();

				ShowRecentProjects(state, projectMgr);

				ImGui::Separator();

				// Browse button
				float buttonWidth = 150.0f;
				float spacing = 10.0f;
				float totalWidth = buttonWidth * 2 + spacing;
				float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

				ImGui::SetCursorPosX(startX);
				if (ImGui::Button("Browse for Project...", ImVec2(buttonWidth, 30))) {
					std::cout << "[ProjectPopups] Browse for project (file dialog not implemented)" << std::endl;
					// TODO: Implement file dialog
				}

				ImGui::SameLine(0, spacing);
				if (ImGui::Button("Refresh", ImVec2(buttonWidth, 30))) {
					state.RefreshRecentProjects(projectMgr);
				}

				ImGui::Separator();

				// Cancel button
				buttonWidth = 100.0f;
				startX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
				ImGui::SetCursorPosX(startX);
				if (ImGui::Button("Cancel", ImVec2(buttonWidth, 30))) {
					state.showLoadProjectPopup = false;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			// Handle close button or ESC key
			if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				state.showLoadProjectPopup = false;
			}
		}

		void ShowTemplateSelector(ProjectPopupState& state) {
			ImGui::Text("Choose Template:");

			if (ImGui::BeginChild("TemplateList", ImVec2(0, 200), true)) {
				// Blank project option
				bool isBlankSelected = (state.selectedTemplate == -1);
				if (ImGui::Selectable("Blank Project", isBlankSelected)) {
					state.selectedTemplate = -1;
				}
				if (isBlankSelected) {
					ImGui::Indent();
					ImGui::TextWrapped("Empty project with no views open");
					ImGui::Unindent();
				}

				if (!state.templates.empty()) {
					ImGui::Separator();
				}

				// Template options
				for (int i = 0; i < state.templates.size(); ++i) {
					const auto& template_ = state.templates[i];
					bool isSelected = (state.selectedTemplate == i);

					if (ImGui::Selectable(template_.name.c_str(), isSelected)) {
						state.selectedTemplate = i;
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

		void ShowRecentProjects(ProjectPopupState& state, ANI::ProjectManager& projectMgr) {
			ImGui::Text("Recent Projects:");

			if (state.recentProjects.empty()) {
				ImGui::TextDisabled("No recent projects found");
				return;
			}

			if (ImGui::BeginChild("RecentProjects", ImVec2(0, 220), true)) {
				for (const auto& projectPath : state.recentProjects) {
					std::filesystem::path path(projectPath);
					std::string displayName = path.filename().string();

					if (ImGui::Selectable(displayName.c_str())) {

						if (projectMgr.LoadProject(projectPath)) {
							std::cout << "[ProjectPopups] Loaded project: " << projectPath << std::endl;
							state.showLoadProjectPopup = false;
							ImGui::CloseCurrentPopup();
						}
						else {
							std::cerr << "[ProjectPopups] Failed to load project: " << projectMgr.GetLastError() << std::endl;
						}
					}

					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("%s", projectPath.c_str());
					}
				}
			}
			ImGui::EndChild();
		}

		void CreateProject(ProjectPopupState& state, ANI::ProjectManager& projectMgr) {
			std::string projectName = std::string(state.projectNameBuffer);
			std::string projectPath = std::string(state.projectPathBuffer);

			if (projectName.empty() || projectPath.empty()) {
				std::cerr << "[ProjectPopups] Project name and path cannot be empty" << std::endl;
				return;
			}

			std::string fullPath = projectPath + "/" + projectName;

			if (projectMgr.CreateNewProject(fullPath, projectName)) {
				std::cout << "[ProjectPopups] Project created successfully: " << projectName << std::endl;

				// Apply template directly if one is selected
				if (state.selectedTemplate >= 0 && state.selectedTemplate < state.templates.size()) {
					const auto& template_ = state.templates[state.selectedTemplate];

					std::cout << "[ProjectPopups] Applying template: " << template_.name << std::endl;

					if (!projectMgr.ApplyProjectTemplate(template_)) {
						std::cerr << "[ProjectPopups] Failed to apply template: " << projectMgr.GetLastError() << std::endl;
					}
					else {
						std::cout << "[ProjectPopups] Template applied successfully" << std::endl;
					}
				}
				else {
					std::cout << "[ProjectPopups] Created blank project (no template selected)" << std::endl;
				}
			}
			else {
				std::cerr << "[ProjectPopups] Failed to create project: " << projectMgr.GetLastError() << std::endl;
			}
		}

	} // namespace ProjectPopups

} // namespace GUI