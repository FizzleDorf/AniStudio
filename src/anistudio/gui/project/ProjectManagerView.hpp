#pragma once

#include "AniStudio.hpp"
#include "ProjectSystem.hpp"
#include "ProjectPopups.hpp"
#include "AutoLoadPopup.hpp"
#include "NetworkStartup.hpp"

namespace GUI {

    class ProjectManagerView {
    private:
        ECS::ProjectSystem& m_projectSystem;
        ANI::StudioCore* m_studioCore;
        ProjectPopupState popupState;
        AutoLoadPopupState autoLoadState;

        NetworkStartup m_networkStartup;
        bool m_networkMode = false;

    public:
        ProjectManagerView(ECS::ProjectSystem& projectSystem, ANI::StudioCore* studioCore);

        void Init();
        void Update(const float deltaT);
        void Render();

        void SetNetworkMode(bool on) { m_networkMode = on; }
        bool IsNetworkMode() const { return m_networkMode; }

        void ShowNewProjectDialog();
        void ShowLoadProjectDialog();
        void ShowAutoLoadPopup(const std::string& lastProjectPath);

        bool IsAutoLoadPopupActive() const { return autoLoadState.showPopup; }
        bool ShouldAutoLoad() const { return autoLoadState.shouldAutoLoad; }

        std::function<void(const NetworkStartupResult&)> onNetworkReady;

    private:
        void RenderAutoLoadPopup();
    };

} // namespace GUI