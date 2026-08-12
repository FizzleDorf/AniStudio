#pragma once

#include "AniStudio.hpp"
#include "ProjectSystem.hpp"
#include "ProjectPopups.hpp"
#include "AutoLoadPopup.hpp"

namespace GUI {

    class ProjectManagerView {
    private:
        ANI::ProjectSystem& m_projectSystem;
        ANI::StudioCore* m_studioCore;
        ProjectPopupState popupState;
        AutoLoadPopupState autoLoadState;

    public:
        ProjectManagerView(ANI::ProjectSystem& projectSystem, ANI::StudioCore* studioCore);

        void Init();
        void Update(const float deltaT);
        void Render();

        void ShowNewProjectDialog();
        void ShowLoadProjectDialog();
        void ShowAutoLoadPopup(const std::string& lastProjectPath);

        bool IsAutoLoadPopupActive() const { return autoLoadState.showPopup; }
        bool ShouldAutoLoad() const { return autoLoadState.shouldAutoLoad; }

    private:
        void RenderAutoLoadPopup();
    };

} // namespace GUI