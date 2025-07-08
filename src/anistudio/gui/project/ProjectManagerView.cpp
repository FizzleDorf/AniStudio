#include "ProjectManagerView.hpp"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <filesystem>

namespace GUI {

	ProjectManagerView::ProjectManagerView(ECS::EntityManager& entityMgr, ANI::ProjectManager& projectMgr)
		: BaseView(entityMgr), m_projectManager(projectMgr) {
		viewName = "Project Manager";
	}

	void ProjectManagerView::Init() {
		std::cout << "[ProjectManagerView] Startup view initialized" << std::endl;
	}

	void ProjectManagerView::Update(const float deltaT) {
		// This view will be closed automatically by ProjectManager when project opens
	}

	void ProjectManagerView::Render() {
		// Center the startup window
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

		ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

		if (ImGui::Begin("Welcome to AniStudio##StartupWindow", nullptr, flags)) {

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
				m_projectManager.GetViewState().SetViewOpen("NewProjectView", true);
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Open Project", ImVec2(buttonWidth, buttonHeight))) {
				m_projectManager.GetViewState().SetViewOpen("LoadProjectView", true);
			}

			ImGui::SameLine(0, spacing);
			if (ImGui::Button("Exit", ImVec2(buttonWidth, buttonHeight))) {
				ANI::Event event;
				event.type = ANI::EventType::Quit;
				ANI::Events::Ref().QueueEvent(event);
			}

		}
		ImGui::End();
	}

} // namespace GUI