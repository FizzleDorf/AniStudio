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

    void SettingsView::Show() {
        m_open = true;
        // Entering the view always starts from the last saved state.
        if (m_settingsSystem) m_settingsSystem->RestoreFromSnapshot();
    }

    void SettingsView::Hide() {
        AttemptClose();
    }

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

        bool windowOpen = true;
        if (ImGui::Begin("Settings##SettingsWindow", &windowOpen, flags)) {
            RenderMainContent();
        }
        ImGui::End();

        RenderCloseConfirmPopup();

        if (!windowOpen) {
            // The X on the title bar.
            AttemptClose();
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

        const bool dirty = m_settingsSystem->HasUnsavedChanges();

        if (dirty) {
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

        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) {
            m_settingsSystem->RestoreFromSnapshot();
        }

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            AttemptClose();
        }
    }

    void SettingsView::RenderCloseConfirmPopup() {
        if (!showCloseConfirm) return;

        ImGui::OpenPopup("Discard Changes##SettingsClose");
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Discard Changes##SettingsClose", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes.");
            ImGui::Text("Discard them and close, or save first?");
            ImGui::Separator();

            if (ImGui::Button("Save and Close", ImVec2(140, 0))) {
                if (m_settingsSystem) m_settingsSystem->SaveAllSettings();
                m_open = false;
                showCloseConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard", ImVec2(140, 0))) {
                if (m_settingsSystem) m_settingsSystem->RestoreFromSnapshot();
                m_open = false;
                showCloseConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(140, 0))) {
                showCloseConfirm = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void SettingsView::AttemptClose() {
        if (!m_settingsSystem) {
            m_open = false;
            return;
        }
        if (m_settingsSystem->HasUnsavedChanges()) {
            showCloseConfirm = true;
            return;
        }
        m_settingsSystem->RestoreFromSnapshot();
        m_open = false;
    }

}