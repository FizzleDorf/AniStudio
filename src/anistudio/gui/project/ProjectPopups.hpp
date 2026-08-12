#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>
#include <imgui.h>

namespace ANI { class ProjectSystem; }

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

        void InitializeBuffers(ANI::ProjectSystem& projectSystem);
        void LoadTemplates(ANI::ProjectSystem& projectSystem);
        void RefreshRecentProjects(ANI::ProjectSystem& projectSystem);

        std::string GenerateDefaultProjectName(ANI::ProjectSystem& projectSystem) const;
    };

    namespace ProjectPopups {
        void RenderNewProjectPopup(ProjectPopupState& state, ANI::ProjectSystem& projectSystem);
        void RenderLoadProjectPopup(ProjectPopupState& state, ANI::ProjectSystem& projectSystem);
        void ShowTemplateSelector(ProjectPopupState& state);
        void ShowRecentProjects(ProjectPopupState& state, ANI::ProjectSystem& projectSystem);
        void CreateProject(ProjectPopupState& state, ANI::ProjectSystem& projectSystem);
    }

} // namespace GUI