#include "NetworkStartup.hpp"
#include "ProjectSystem.hpp"
#include "FileDialogUtil.hpp"

#include <imgui.h>
#include <filesystem>
#include <iostream>
#include <thread>

namespace GUI {

    void NetworkStartup::Begin(bool hostMode) {
        m_active = true;
        m_hostMode = hostMode;
        m_page = Page::Username;
        m_usernameValid = (m_usernameBuffer[0] != 0);
        m_recentSelected = -1;
        m_discoveredSelected = -1;
        m_discoveryDone = false;
        m_discoveryInFlight = false;
        m_recentProjects.clear();
    }

    bool NetworkStartup::Render(ECS::ProjectSystem& projectSystem, NetworkStartupResult& out) {
        if (!m_active) return false;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 460), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_Modal;

        const char* title = m_hostMode
            ? "Host Project##NetworkStartup"
            : "Join Server##NetworkStartup";

        if (ImGui::BeginPopupModal(title, nullptr, flags)) {
            switch (m_page) {
            case Page::Username:     RenderUsernamePage(projectSystem);     break;
            case Page::HostProject:  RenderHostProjectPage(projectSystem);  break;
            case Page::JoinList:     RenderJoinListPage(projectSystem);     break;
            case Page::Confirm:      RenderConfirmPage(projectSystem);      break;
            }
            ImGui::EndPopup();
        }
        else {
            ImGui::OpenPopup(title);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && m_page == Page::Username) {
            m_active = false;
            out.mode = NetworkStartupResult::Mode::None;
            return false;
        }

        if (!m_active) {
            out.username = m_usernameBuffer;
            if (m_hostMode) {
                out.mode = NetworkStartupResult::Mode::Host;
                if (m_hostExisting && m_recentSelected >= 0 && m_recentSelected < (int)m_recentProjects.size()) {
                    out.hostProjectPath = m_recentProjects[m_recentSelected];
                }
                else {
                    out.hostProjectName = m_newProjectName;
                    out.hostCreatePath = m_newProjectPath;
                }
            }
            else {
                out.mode = NetworkStartupResult::Mode::Join;
                if (m_discoveredSelected >= 0 && m_discoveredSelected < (int)m_discovered.size()) {
                    out.joinAddress = m_discovered[m_discoveredSelected].address;
                    out.joinPort = m_discovered[m_discoveredSelected].tcpPort;
                    out.joinProjectName = m_discovered[m_discoveredSelected].projectName;
                }
                else {
                    out.joinAddress = m_manualAddress;
                    out.joinPort = (uint16_t)m_manualPort;
                }
            }
            return true;
        }

        return false;
    }

    void NetworkStartup::RenderUsernamePage(ECS::ProjectSystem& projectSystem) {
        ImGui::Text("Welcome to AniStudio Networking");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("Enter a display name. Other users will see this next to your cursor and edits.");
        ImGui::Spacing();

        ImGui::Text("Username:");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##username", m_usernameBuffer, sizeof(m_usernameBuffer))) {
            m_usernameValid = (m_usernameBuffer[0] != 0);
        }

        ImGui::Spacing();
        ImGui::TextDisabled("(You can change this later in Settings)");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginDisabled(!m_usernameValid);
        if (ImGui::Button("Continue", ImVec2(150, 32))) {
            m_page = m_hostMode ? Page::HostProject : Page::JoinList;
            if (!m_hostMode) {
                m_discoveryInFlight = true;
                m_discoveryDone = false;
                std::thread([this]() {
                    auto list = Net::DiscoverServers();
                    m_discovered = std::move(list);
                    m_discoveryInFlight = false;
                    m_discoveryDone = true;
                    }).detach();
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 32))) {
            m_active = false;
            ImGui::CloseCurrentPopup();
        }
    }

    void NetworkStartup::RenderHostProjectPage(ECS::ProjectSystem& projectSystem) {
        ImGui::Text("Host a Project");
        ImGui::Separator();
        ImGui::TextDisabled("Username: %s", m_usernameBuffer);

        ImGui::Spacing();

        if (ImGui::RadioButton("Create a new project", !m_hostExisting)) {
            m_hostExisting = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Open an existing project", m_hostExisting)) {
            m_hostExisting = true;
            m_recentProjects = projectSystem.GetRecentProjects();
        }

        ImGui::Separator();

        if (!m_hostExisting) {
            ImGui::Text("New project name:");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##newName", m_newProjectName, sizeof(m_newProjectName));

            ImGui::Text("Location:");
            ImGui::SetNextItemWidth(-160);
            ImGui::InputText("##newPath", m_newProjectPath, sizeof(m_newProjectPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse...", ImVec2(140, 0))) {
                std::string picked;
                if (FileDialog::SelectFolder("Choose project location", picked)) {
                    strncpy(m_newProjectPath, picked.c_str(), sizeof(m_newProjectPath) - 1);
                    m_newProjectPath[sizeof(m_newProjectPath) - 1] = 0;
                }
            }
        }
        else {
            ImGui::Text("Recent projects:");
            if (ImGui::BeginChild("HostRecentList", ImVec2(0, 160), true)) {
                if (m_recentProjects.empty()) {
                    ImGui::TextDisabled("No recent projects.");
                }
                for (int i = 0; i < (int)m_recentProjects.size(); ++i) {
                    std::string label = std::filesystem::path(m_recentProjects[i]).filename().string();
                    if (ImGui::Selectable(label.c_str(), m_recentSelected == i)) {
                        m_recentSelected = i;
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", m_recentProjects[i].c_str());
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();
            if (ImGui::Button("Browse...")) {
                std::string picked;
                if (FileDialog::SelectFolder("Choose project to host", picked)) {
                    m_recentProjects.push_back(picked);
                    m_recentSelected = (int)m_recentProjects.size() - 1;
                }
            }
        }

        ImGui::Separator();

        bool canHost = false;
        if (m_hostExisting) {
            canHost = m_recentSelected >= 0 && m_recentSelected < (int)m_recentProjects.size();
        }
        else {
            canHost = m_newProjectName[0] != 0 && m_newProjectPath[0] != 0;
        }

        ImGui::BeginDisabled(!canHost);
        if (ImGui::Button("Host && Join", ImVec2(160, 34))) {
            m_page = Page::Confirm;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Back", ImVec2(120, 34))) {
            m_page = Page::Username;
        }
    }

    void NetworkStartup::RenderJoinListPage(ECS::ProjectSystem& projectSystem) {
        ImGui::Text("Join a Server");
        ImGui::Separator();
        ImGui::TextDisabled("Username: %s", m_usernameBuffer);

        ImGui::Spacing();

        if (m_discoveryInFlight) {
            ImGui::Text("Scanning LAN for servers...");
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                m_discoveryInFlight = true;
                std::thread([this]() {
                    auto list = Net::DiscoverServers();
                    m_discovered = std::move(list);
                    m_discoveryInFlight = false;
                    m_discoveryDone = true;
                    }).detach();
            }
        }
        else {
            if (ImGui::Button("Refresh")) {
                m_discoveryInFlight = true;
                std::thread([this]() {
                    auto list = Net::DiscoverServers();
                    m_discovered = std::move(list);
                    m_discoveryInFlight = false;
                    m_discoveryDone = true;
                    }).detach();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("Found %zu server(s)", m_discovered.size());
        }

        if (ImGui::BeginChild("JoinServerList", ImVec2(0, 180), true)) {
            if (m_discoveryDone && m_discovered.empty()) {
                ImGui::TextDisabled("No servers found on the LAN.");
                ImGui::TextDisabled("Use the direct-connect field below.");
            }
            for (int i = 0; i < (int)m_discovered.size(); ++i) {
                const auto& s = m_discovered[i];
                std::string label = s.projectName
                    + "  -  " + s.host
                    + " (" + s.address + ":" + std::to_string(s.tcpPort) + ")"
                    + "  [" + std::to_string(s.clientCount) + "/"
                    + std::to_string(s.maxClients) + "]";

                if (ImGui::Selectable(label.c_str(), m_discoveredSelected == i)) {
                    m_discoveredSelected = i;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "Project: %s\nHost user: %s\nAddress: %s:%u\nClients: %u/%u",
                        s.projectName.c_str(),
                        s.hostUser.c_str(),
                        s.address.c_str(), s.tcpPort,
                        s.clientCount, s.maxClients);
                }
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Direct connect (if not on same LAN):");
        ImGui::SetNextItemWidth(220);
        ImGui::InputText("##manualAddr", m_manualAddress, sizeof(m_manualAddress));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("Port##manualPort", &m_manualPort);

        ImGui::Separator();

        bool haveSelection =
            (m_discoveredSelected >= 0 && m_discoveredSelected < (int)m_discovered.size())
            || m_manualAddress[0] != 0;

        ImGui::BeginDisabled(!haveSelection);
        if (ImGui::Button("Join", ImVec2(140, 34))) {
            m_page = Page::Confirm;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Back", ImVec2(120, 34))) {
            m_page = Page::Username;
        }
    }

    void NetworkStartup::RenderConfirmPage(ECS::ProjectSystem& projectSystem) {
        ImGui::Text("Ready");
        ImGui::Separator();

        if (m_hostMode) {
            ImGui::Text("Username:   %s", m_usernameBuffer);
            if (m_hostExisting && m_recentSelected >= 0 && m_recentSelected < (int)m_recentProjects.size()) {
                ImGui::Text("Project:    %s", m_recentProjects[m_recentSelected].c_str());
            }
            else {
                ImGui::Text("New project: %s", m_newProjectName);
                ImGui::Text("Location:    %s", m_newProjectPath);
            }
            ImGui::Spacing();
            ImGui::TextWrapped(
                "A local server will be started for this project and the "
                "client will connect to it. Other users on your LAN will see "
                "it when they choose \"Join\".");
        }
        else {
            ImGui::Text("Username:   %s", m_usernameBuffer);
            if (m_discoveredSelected >= 0 && m_discoveredSelected < (int)m_discovered.size()) {
                const auto& s = m_discovered[m_discoveredSelected];
                ImGui::Text("Server:     %s", s.host.c_str());
                ImGui::Text("Address:    %s:%u", s.address.c_str(), s.tcpPort);
                ImGui::Text("Project:    %s", s.projectName.c_str());
            }
            else {
                ImGui::Text("Address:    %s:%d", m_manualAddress, m_manualPort);
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Confirm", ImVec2(140, 34))) {
            m_active = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Back", ImVec2(120, 34))) {
            m_page = m_hostMode ? Page::HostProject : Page::JoinList;
        }
    }

} // namespace GUI