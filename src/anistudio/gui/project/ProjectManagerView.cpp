#include "ProjectManagerView.hpp"
#include "ProjectManager.hpp"
#include "SettingsView.hpp"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace GUI {

    ProjectManagerView::ProjectManagerView(ANI::ProjectManager& projectMgr, ANI::StudioCore* studioCore)
        : m_projectManager(projectMgr), m_studioCore(studioCore) {
        std::cout << "[ProjectManagerView] Constructor - StudioCore: " << m_studioCore << std::endl;
    }

    void ProjectManagerView::Init() {
        std::cout << "[ProjectManagerView] Startup view initialized" << std::endl;

        popupState.InitializeBuffers(m_projectManager);
        popupState.LoadTemplates(m_projectManager);
        popupState.RefreshRecentProjects(m_projectManager);

        autoLoadState.showPopup = false;
        autoLoadState.userChoiceMade = false;
        autoLoadState.shouldAutoLoad = false;
    }

    void ProjectManagerView::Update(const float deltaT) {
        if (m_projectManager.IsProjectOpen()) {
            popupState.showNewProjectPopup = false;
            popupState.showLoadProjectPopup = false;
            autoLoadState.showPopup = false;
        }
    }

    void ProjectManagerView::Render() {
        if (m_projectManager.IsProjectOpen()) {
            return;
        }

        if (autoLoadState.showPopup) {
            RenderAutoLoadPopup();
            if (autoLoadState.showPopup) {
                return;
            }
        }

        ProjectPopups::RenderNewProjectPopup(popupState, m_projectManager);
        ProjectPopups::RenderLoadProjectPopup(popupState, m_projectManager);

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

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
                        std::string fullPath = path.string();

                        if (ImGui::Selectable(displayName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                if (m_projectManager.LoadProject(fullPath)) {
                                    std::cout << "[ProjectManagerView] Loaded project: " << fullPath << std::endl;
                                }
                                else {
                                    std::cerr << "[ProjectManagerView] Failed to load project: " << m_projectManager.GetLastError() << std::endl;
                                }
                            }
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Path: %s\nDouble-click to open", fullPath.c_str());
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            float buttonHeight = 48.0f;
            float textHeight = ImGui::GetFontSize();
            float verticalPadding = (buttonHeight - textHeight) / 2.0f;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, verticalPadding));

            if (ImGui::Button("New Project", ImVec2(-FLT_MIN, buttonHeight))) {
                popupState.showNewProjectPopup = true;
                std::cout << "[ProjectManagerView] Opening New Project dialog" << std::endl;
            }

            if (ImGui::Button("Open Project", ImVec2(-FLT_MIN, buttonHeight))) {
                popupState.showLoadProjectPopup = true;
                popupState.RefreshRecentProjects(m_projectManager);
                std::cout << "[ProjectManagerView] Opening Load Project dialog" << std::endl;
            }

            if (ImGui::Button("Settings", ImVec2(-FLT_MIN, buttonHeight))) {
                if (m_studioCore) {
                    m_studioCore->GetSettingsView().Show();
                    std::cout << "[ProjectManagerView] Opening Settings dialog" << std::endl;
                }
                else {
                    std::cerr << "[ProjectManagerView] ERROR: Cannot open settings - StudioCore is null!" << std::endl;
                }
            }

            if (ImGui::Button("Exit", ImVec2(-FLT_MIN, buttonHeight))) {
                std::cout << "[ProjectManagerView] Exiting application" << std::endl;
                exit(0);
            }

            ImGui::PopStyleVar();
        }
        ImGui::End();
    }

    void ProjectManagerView::RenderAutoLoadPopup() {
        if (AutoLoadPopup::Show(autoLoadState)) {
            if (autoLoadState.shouldAutoLoad) {
                std::cout << "[ProjectManagerView] User chose to auto-load project: "
                    << autoLoadState.lastProjectPath << std::endl;

                if (m_projectManager.LoadProject(autoLoadState.lastProjectPath)) {
                    std::cout << "[ProjectManagerView] Auto-load successful" << std::endl;
                }
                else {
                    std::cerr << "[ProjectManagerView] Auto-load failed: "
                        << m_projectManager.GetLastError() << std::endl;
                }
            }
            else {
                std::cout << "[ProjectManagerView] User chose to show project manager" << std::endl;
            }
        }
    }

    void ProjectManagerView::ShowAutoLoadPopup(const std::string& lastProjectPath) {
        if (AutoLoadPopup::ShouldShow(lastProjectPath)) {
            autoLoadState.showPopup = true;
            autoLoadState.lastProjectPath = lastProjectPath;
            autoLoadState.lastProjectName = AutoLoadPopup::GetProjectNameFromPath(lastProjectPath);
            autoLoadState.userChoiceMade = false;
            autoLoadState.shouldAutoLoad = false;

            std::cout << "[ProjectManagerView] Showing auto-load popup for project: "
                << autoLoadState.lastProjectName << std::endl;
        }
    }

    void ProjectManagerView::ShowNewProjectDialog() {
        popupState.showNewProjectPopup = true;
        std::cout << "[ProjectManagerView] ShowNewProjectDialog called" << std::endl;
    }

    void ProjectManagerView::ShowLoadProjectDialog() {
        popupState.showLoadProjectPopup = true;
        popupState.RefreshRecentProjects(m_projectManager);
        std::cout << "[ProjectManagerView] ShowLoadProjectDialog called" << std::endl;
    }

} // namespace GUI