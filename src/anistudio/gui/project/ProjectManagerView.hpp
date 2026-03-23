#pragma once

#include "AniStudio.hpp"
#include "ProjectManager.hpp"
#include "ProjectPopups.hpp"
#include "AutoLoadPopup.hpp"
#include "SettingsView.hpp"

namespace GUI {

    class ProjectManagerView {
    private:
        ANI::ProjectManager& m_projectManager;
        ProjectPopupState popupState;
        AutoLoadPopupState autoLoadState;
        SettingsView settingsView;

    public:
        ProjectManagerView(ANI::ProjectManager& projectMgr);

        void Init();
        void Update(const float deltaT);
        void Render();

        void ShowNewProjectDialog();
        void ShowLoadProjectDialog();
        void ShowAutoLoadPopup(const std::string& lastProjectPath);
        void ShowSettingsDialog() { settingsView.Show(); }

        bool IsAutoLoadPopupActive() const { return autoLoadState.showPopup; }
        bool ShouldAutoLoad() const { return autoLoadState.shouldAutoLoad; }

    private:
        void RenderAutoLoadPopup();
    };

} // namespace GUI