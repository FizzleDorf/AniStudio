#include "ProjectManagerView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace GUI {

	ProjectManagerView::ProjectManagerView(ANI::ProjectManager& projectMgr)
		: m_projectManager(projectMgr) {
	}

	void ProjectManagerView::Init() {
		std::cout << "[ProjectManagerView] Startup view initialized" << std::endl;

		// Initialize popup state
		popupState.InitializeBuffers(m_projectManager);
		popupState.LoadTemplates();
		popupState.RefreshRecentProjects(m_projectManager);
	}

	void ProjectManagerView::Update(const float deltaT) {
		// Close this view if a project is now open
		if (m_projectManager.IsProjectOpen()) {
			// View will be hidden automatically when project opens
			// Reset popup flags when closing
			popupState.showNewProjectPopup = false;
			popupState.showLoadProjectPopup = false;
		}
	}

	void ProjectManagerView::Render() {
		// Don't render if project is open
		if (m_projectManager.IsProjectOpen()) {
			return;
		}

		// Render project popups first - same as MenuBar!
		ProjectPopups::RenderNewProjectPopup(popupState, m_projectManager);
		ProjectPopups::RenderLoadProjectPopup(popupState, m_projectManager);

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
				popupState.showNewProjectPopup = true;
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Open Project", ImVec2(buttonWidth, buttonHeight))) {
				popupState.showLoadProjectPopup = true;
				popupState.RefreshRecentProjects(m_projectManager);
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
	}

	// Public methods for MenuBar to trigger popups
	void ProjectManagerView::ShowNewProjectDialog() {
		popupState.showNewProjectPopup = true;
	}

	void ProjectManagerView::ShowLoadProjectDialog() {
		popupState.showLoadProjectPopup = true;
		popupState.RefreshRecentProjects(m_projectManager);
	}

} // namespace GUI