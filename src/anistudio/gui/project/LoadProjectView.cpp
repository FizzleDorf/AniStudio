#include "LoadProjectView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>

namespace GUI {

	LoadProjectView::LoadProjectView(ECS::EntityManager& entityMgr, ANI::ProjectManager& projectMgr)
		: BaseView(entityMgr), m_projectManager(projectMgr) {
		viewName = "Load Project";
	}

	void LoadProjectView::Init() {
		RefreshRecentProjects();
	}

	void LoadProjectView::Update(const float deltaT) {
		// Nothing to update
	}

	void LoadProjectView::Render() {
		if (ImGui::Begin(viewName.c_str())) {
			ImGui::Text("Load Project");
			ImGui::Separator();

			ShowRecentProjects();

			ImGui::Separator();

			ShowBrowseOption();

			ImGui::Separator();

			if (ImGui::Button("Cancel")) {
				// TODO: Close this view
			}
		}
		ImGui::End();
	}

	void LoadProjectView::RefreshRecentProjects() {
		m_recentProjects = m_projectManager.GetRecentProjects();
	}

	void LoadProjectView::ShowRecentProjects() {
		ImGui::Text("Recent Projects:");

		if (m_recentProjects.empty()) {
			ImGui::TextDisabled("No recent projects found");
			return;
		}

		// Show recent projects in a child window for scrolling
		if (ImGui::BeginChild("RecentProjects", ImVec2(0, 200), true)) {
			for (const auto& projectPath : m_recentProjects) {
				std::filesystem::path path(projectPath);
				std::string displayName = path.filename().string();

				if (ImGui::Selectable(displayName.c_str())) {
					if (m_projectManager.LoadProject(projectPath)) {
						// TODO: Close this view
						std::cout << "[LoadProjectView] Loaded project: " << projectPath << std::endl;
					}
					else {
						std::cerr << "[LoadProjectView] Failed to load project: " << m_projectManager.GetLastError() << std::endl;
					}
				}

				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("%s", projectPath.c_str());
				}
			}
		}
		ImGui::EndChild();
	}

	void LoadProjectView::ShowBrowseOption() {
		if (ImGui::Button("Browse for Project...", ImVec2(200, 30))) {
			// TODO: File dialog to browse for .ani project files
			std::cout << "[LoadProjectView] Browse for project (file dialog not implemented)" << std::endl;
		}

		ImGui::SameLine();

		if (ImGui::Button("Refresh", ImVec2(80, 30))) {
			RefreshRecentProjects();
		}
	}

} // namespace GUI