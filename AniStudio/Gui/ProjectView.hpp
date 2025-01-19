#pragma once
#include "Base/BaseView.hpp"
#include "ImGuiFileDialog.h"
#include "filepaths.hpp"
#include <filesystem>

namespace GUI {

class ProjectView : public BaseView {
public:
    ProjectView(ECS::EntityManager &entityMgr) : BaseView(entityMgr) { 
    viewName = "ProjectView"; 
    mgr.RegisterSystem<ProjectSystem>();
    }

    void Init() override { projectSystem = mgr.GetSystem<ECS::ProjectSystem>(); }

    void Render() override {
        if (ImGui::Begin("Project Manager")) {
            if (ImGui::Button("New Project")) {
                IGFD::FileDialogConfig config;
                config.path = filePaths.defaultProjectPath;
                ImGuiFileDialog::Instance()->OpenDialog("NewProjectDialog", "Choose Project Location", nullptr, config);
            }

            ImGui::SameLine();
            if (ImGui::Button("Open Project")) {
                IGFD::FileDialogConfig config;
                config.path = filePaths.defaultProjectPath;
                ImGuiFileDialog::Instance()->OpenDialog("OpenProjectDialog", "Choose Project", nullptr, config);
            }

            ImGui::SameLine();
            if (ImGui::Button("Save Project")) {
                if (!filePaths.lastOpenProjectPath.empty()) {
                    SaveProject(filePaths.lastOpenProjectPath);
                }
            }

            ImGui::Text("Current Project: %s",
                        filePaths.lastOpenProjectPath.empty() ? "<None>" : filePaths.lastOpenProjectPath.c_str());

            HandleDialogs();
        }
        ImGui::End();
    }

private:
    void HandleDialogs() {
        // New Project Dialog
        if (ImGuiFileDialog::Instance()->Display("NewProjectDialog")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string projectPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                CreateNewProject(projectPath);
            }
            ImGuiFileDialog::Instance()->Close();
        }

        // Open Project Dialog
        if (ImGuiFileDialog::Instance()->Display("OpenProjectDialog")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string projectPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                OpenProject(projectPath);
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    void CreateNewProject(const std::string &path) {
        if (projectSystem && projectSystem->CreateNewProject(path)) {
            filePaths.lastOpenProjectPath = path;
            filePaths.SaveFilepathDefaults();
        }
    }

    void OpenProject(const std::string &path) {
        if (projectSystem && projectSystem->LoadProject(path)) {
            UpdateDiffusionView();
        }
    }

    void SaveProject(const std::string &path) {
        if (projectSystem) {
            projectSystem->SaveProject(path);
        }
    }

    void UpdateDiffusionView() {
        // auto diffusionView = viewManager.GetView<DiffusionView>();
        // if (!diffusionView)
        //     return;

        // Update DiffusionView components based on loaded project data
        for (const auto &entity : mgr.GetAllEntities()) {
            if (mgr.HasComponent<ImageComponent>(entity)) {
                auto &imageComp = mgr.GetComponent<ImageComponent>(entity);
            }
        }
    }

    std::shared_ptr<ECS::ProjectSystem> projectSystem;
};

} // namespace GUI