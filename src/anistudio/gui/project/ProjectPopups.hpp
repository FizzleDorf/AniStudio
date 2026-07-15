#pragma once

#include "ProjectTemplate.hpp"
#include <vector>
#include <string>
#include <imgui.h>

namespace ANI { class ProjectManager; }

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

        void InitializeBuffers(ANI::ProjectManager& projectMgr);
        void LoadTemplates(ANI::ProjectManager& projectMgr);      // now takes a reference
        void RefreshRecentProjects(ANI::ProjectManager& projectMgr);

        std::string GenerateDefaultProjectName(ANI::ProjectManager& projectMgr) const;
    };

    namespace ProjectPopups {
        void RenderNewProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
        void RenderLoadProjectPopup(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
        void ShowTemplateSelector(ProjectPopupState& state);
        void ShowRecentProjects(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
        void CreateProject(ProjectPopupState& state, ANI::ProjectManager& projectMgr);
    }

} // namespace GUI