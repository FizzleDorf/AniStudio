#include "LoadProjectView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>

namespace GUI {

	LoadProjectView::LoadProjectView(ANI::ProjectManager& projectMgr)
		: m_projectManager(projectMgr) {
	}

	void LoadProjectView::Init() {
		RefreshRecentProjects();
	}

	void LoadProjectView::Update(const float deltaT) {
		// Close popup if a project was loaded
		if (m_projectManager.IsProjectOpen()) {
			m_showPopup = false;
		}
	}

	void LoadProjectView::Render() {
		if (!m_showPopup) {
			return;
		}

		if (m_showPopup) {
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

			ShowRecentProjects();

			ImGui::Separator();

			ShowBrowseOption();

			ImGui::Separator();

			// Cancel button
			float buttonWidth = 100.0f;
			float startX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
			ImGui::SetCursorPosX(startX);
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

	void LoadProjectView::RefreshRecentProjects() {
		m_recentProjects = m_projectManager.GetRecentProjects();
	}

	void LoadProjectView::ShowRecentProjects() {
		ImGui::Text("Recent Projects:");

		if (m_recentProjects.empty()) {
			ImGui::TextDisabled("No recent projects found");
			return;
		}

		if (ImGui::BeginChild("RecentProjects", ImVec2(0, 220), true)) {
			for (const auto& projectPath : m_recentProjects) {
				std::filesystem::path path(projectPath);
				std::string displayName = path.filename().string();

				if (ImGui::Selectable(displayName.c_str())) {
					if (m_projectManager.LoadProject(projectPath)) {
						std::cout << "[LoadProjectView] Loaded project: " << projectPath << std::endl;
						m_showPopup = false;
						ImGui::CloseCurrentPopup();
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
		float buttonWidth = 150.0f;
		float spacing = 10.0f;
		float totalWidth = buttonWidth * 2 + spacing;
		float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

		ImGui::SetCursorPosX(startX);
		if (ImGui::Button("Browse for Project...", ImVec2(buttonWidth, 30))) {
			std::cout << "[LoadProjectView] Browse for project (file dialog not implemented)" << std::endl;
		}

		ImGui::SameLine(0, spacing);
		if (ImGui::Button("Refresh", ImVec2(buttonWidth, 30))) {
			RefreshRecentProjects();
		}
	}

} // namespace GUI