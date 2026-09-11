#pragma once
#include "Discovery.hpp"
#include <string>
#include <vector>
#include <functional>

namespace ECS { class ProjectSystem; }

namespace GUI {

    struct NetworkStartupResult {
        enum class Mode { None, Host, Join } mode = Mode::None;
        std::string username;
        std::string hostProjectPath;
        std::string hostProjectName;
        std::string hostCreatePath;
        std::string joinAddress;
        uint16_t    joinPort = 9000;
        std::string joinProjectName;
    };

    class NetworkStartup {
    public:
        bool Render(ECS::ProjectSystem& projectSystem, NetworkStartupResult& out);
        void Begin(bool hostMode);
        bool IsActive() const { return m_active; }

    private:
        enum class Page { Username, HostProject, JoinList, Confirm };

        void RenderUsernamePage(ECS::ProjectSystem&);
        void RenderHostProjectPage(ECS::ProjectSystem&);
        void RenderJoinListPage(ECS::ProjectSystem&);
        void RenderConfirmPage(ECS::ProjectSystem&);

        bool m_active = false;
        bool m_hostMode = true;
        Page m_page = Page::Username;

        char m_usernameBuffer[64] = { 0 };
        bool m_usernameValid = false;

        char m_newProjectName[128] = { 0 };
        char m_newProjectPath[512] = { 0 };
        bool m_hostExisting = false;
        std::vector<std::string> m_recentProjects;
        int m_recentSelected = -1;

        std::vector<Net::ServerInfo> m_discovered;
        int m_discoveredSelected = -1;
        bool m_discoveryDone = false;
        bool m_discoveryInFlight = false;
        char m_manualAddress[128] = { 0 };
        int  m_manualPort = 9000;
    };

} // namespace GUI