#include "ProjectManagerView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>

namespace GUI {

	ProjectManagerView::ProjectManagerView(ECS::EntityManager& entityMgr, ANI::ProjectManager& projectMgr)
		: BaseView(entityMgr), m_projectManager(projectMgr) {
		viewName = "Project Manager";
	}

	void ProjectManagerView::Init() {
		// This view shows when no project is open
	}

	void ProjectManagerView::Update(const float deltaT) {
		// Close this view if a project is opened
		if (m_projectManager.IsProjectOpen()) {
			// TODO: Close this view
		}
	}

	void ProjectManagerView::Render() {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

		if (ImGui::Begin("Welcome to AniStudio", nullptr, flags)) {
			ImGui::Text("No project is currently open.");
			ImGui::Separator();

			ImVec2 buttonSize(150, 40);

			if (ImGui::Button("Create New Project", buttonSize)) {
				m_showNewProjectDialog = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Load Project", buttonSize)) {
				m_showLoadProjectDialog = true;
			}

			ImGui::Separator();

			// Recent projects
			ImGui::Text("Recent Projects:");
			auto recentProjects = m_projectManager.GetRecentProjects();

			if (recentProjects.empty()) {
				ImGui::TextDisabled("No recent projects");
			}
			else {
				for (const auto& projectPath : recentProjects) {
					std::filesystem::path path(projectPath);
					std::string displayName = path.filename().string();

					if (ImGui::Selectable(displayName.c_str())) {
						m_projectManager.LoadProject(projectPath);
					}

					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("%s", projectPath.c_str());
					}
				}
			}
		}
		ImGui::End();

		// Handle dialog flags
		if (m_showNewProjectDialog) {
			// Open new project view
			// TODO: Get ViewState and open NewProjectView
			m_showNewProjectDialog = false;
		}

		if (m_showLoadProjectDialog) {
			// Open load project view  
			// TODO: Get ViewState and open LoadProjectView
			m_showLoadProjectDialog = false;
		}
	}

} // namespace GUI