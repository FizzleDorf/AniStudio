#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>
#include <imgui.h>

namespace ECS { class ProjectSystem; }

namespace GUI {

    struct ProjectPopupState {
        bool showNewProjectPopup = false;
        bool showLoadProjectPopup = false;

        char projectNameBuffer[256] = { 0 };
        char projectPathBuffer[512] = { 0 };
        std::vector<ProjectTemplate> templates;
        int selectedTemplate = -1;
        std::vector<std::string> recentProjects;

        ProjectPopupState() = default;

        void InitializeBuffers(ECS::ProjectSystem& projectSystem);
        void LoadTemplates(ECS::ProjectSystem& projectSystem);
        void RefreshRecentProjects(ECS::ProjectSystem& projectSystem);

        std::string GenerateDefaultProjectName(ECS::ProjectSystem& projectSystem) const;
    };

    namespace ProjectPopups {
        void RenderNewProjectPopup(ProjectPopupState& state, ECS::ProjectSystem& projectSystem);
        void RenderLoadProjectPopup(ProjectPopupState& state, ECS::ProjectSystem& projectSystem);
        void ShowTemplateSelector(ProjectPopupState& state);
        void ShowRecentProjects(ProjectPopupState& state, ECS::ProjectSystem& projectSystem);
        void CreateProject(ProjectPopupState& state, ECS::ProjectSystem& projectSystem);
    }

} // namespace GUI