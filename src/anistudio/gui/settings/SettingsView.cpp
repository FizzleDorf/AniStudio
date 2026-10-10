#include "SettingsView.hpp"
#include "EntityManager.hpp"
#include "SettingsSystem.hpp"
#include "BaseSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>

namespace GUI {

    SettingsView::SettingsView() {
        filterBuffer[0] = '\0';
    }

    void SettingsView::SetEntityManager(ECS::EntityManager& mgr) {
        m_entityManager = &mgr;
        m_settingsSystem = mgr.GetSystem<ECS::SettingsSystem>().get();
    }

    void SettingsView::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
    }

    void SettingsView::Show() { m_open = true; }
    void SettingsView::Hide() { m_open = false; }

    void SettingsView::Render() {
        if (!m_open) return;
        if (!imguiContext) return;

        ImGui::SetCurrentContext(imguiContext);

        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_Appearing);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoDocking;

        if (ImGui::Begin("Settings##SettingsWindow", &m_open, flags)) {
            RenderMainContent();
        }
        ImGui::End();

        if (!m_open && m_settingsSystem) {
            m_settingsSystem->RestoreAllFromBackups();
        }
    }

    void SettingsView::RenderMainContent() {
        if (!m_settingsSystem) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "SettingsSystem not available!");
            return;
        }

        ImGui::BeginChild("SettingsArea", ImVec2(0, -50), false);

        ImGui::Text("Search");
        ImGui::SameLine();
        ImGui::InputText("##SettingsFilter", filterBuffer, sizeof(filterBuffer));

        std::string filterStr = filterBuffer;
        if (m_tabs) {
            for (const auto& tab : *m_tabs) {
                if (tab) tab->SetFilter(filterStr);
            }
        }

        if (showSaveNotification) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Settings saved.");
            showSaveNotification = false;
        }

        ImGui::Separator();
        RenderTabsAndContent();
        ImGui::EndChild();

        ImGui::Separator();
        RenderActionButtons();
    }

    void SettingsView::RenderTabsAndContent() {
        if (!m_tabs || m_tabs->empty()) {
            ImGui::TextDisabled("No settings tabs available.");
            return;
        }

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
            for (const auto& tab : *m_tabs) {
                if (!tab) continue;
                std::string title = tab->GetTabName() + " Settings";
                if (ImGui::BeginTabItem(title.c_str())) {
                    ImGui::BeginChild("TabContent", ImVec2(0, 0), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar);
                    tab->Render();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    void SettingsView::RenderActionButtons() {
        if (!m_settingsSystem) return;

        if (m_settingsSystem->HasAnyUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "* Unsaved changes");
            ImGui::SameLine();
        }

        if (ImGui::Button("Apply")) {
            if (m_settingsSystem->SaveAllSettings()) showSaveNotification = true;
        }
        ImGui::SameLine();

        if (ImGui::Button("Save and Close")) {
            if (m_settingsSystem->SaveAllSettings()) m_open = false;
        }

        if (m_settingsSystem->HasAnyUnsavedChanges()) {
            ImGui::SameLine();
            if (ImGui::Button("Cancel Changes")) {
                m_settingsSystem->RestoreAllFromBackups();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            m_settingsSystem->RestoreAllFromBackups();
            m_open = false;
        }
    }

}