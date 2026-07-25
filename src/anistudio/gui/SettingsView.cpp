#include "SettingsView.hpp"
#include "EntityManager.hpp"
#include "SettingsSystem.hpp"
#include "BaseSettingsTab.hpp"
#include <imgui.h>
#include <iostream>
#include <algorithm>

namespace GUI {

    SettingsView::SettingsView() {
        filterBuffer[0] = '\0';
    }

    void SettingsView::SetEntityManager(ECS::EntityManager& mgr) {
        m_entityManager = &mgr;
        m_settingsSystem = mgr.GetSystem<ECS::SettingsSystem>().get();
        if (m_settingsSystem) {
            if (imguiContext) {
                m_settingsSystem->SetImGuiContext(imguiContext);
            }
            if (!settingsLoaded) {
                m_settingsSystem->LoadAllSettings();
                settingsLoaded = true;
            }
            auto& tabs = m_settingsSystem->GetTabs();
            if (!tabs.empty()) {
                currentActiveTab = tabs[0]->GetTabName();
            }
        }
    }

    void SettingsView::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
        if (m_settingsSystem && imguiContext) {
            m_settingsSystem->SetImGuiContext(imguiContext);
        }
    }

    void SettingsView::Render() {
        if (!showPopup) return;
        if (!imguiContext) return;

        ImGui::SetCurrentContext(imguiContext);
        ImGui::OpenPopup("Settings##SettingsPopup");

        const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_Appearing);

        bool isOpen = true;
        if (ImGui::BeginPopupModal("Settings##SettingsPopup", &isOpen,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_Modal)) {
            RenderMainContent();
            ImGui::EndPopup();
        }

        if (!isOpen || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            HandlePopupClose();
        }

        RenderUnsavedChangesDialog();
    }

    void SettingsView::HandlePopupClose() {
        if (m_settingsSystem && m_settingsSystem->HasAnyUnsavedChanges()) {
            showUnsavedChangesDialog = true;
            showPopup = false;
            pendingClose = true;
        }
        else {
            showPopup = false;
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
        for (auto& tab : m_settingsSystem->GetTabs()) {
            tab->SetFilter(filterStr);
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
        auto& tabs = m_settingsSystem->GetTabs();
        if (tabs.empty()) {
            ImGui::TextDisabled("No settings tabs available.");
            return;
        }

        if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
            for (auto& tab : tabs) {
                std::string title = tab->GetTabName() + " Settings";
                if (ImGui::BeginTabItem(title.c_str())) {
                    if (currentActiveTab != tab->GetTabName()) {
                        currentActiveTab = tab->GetTabName();
                    }
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
            if (m_settingsSystem->SaveAllSettings()) {
                showSaveNotification = true;
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Save and Close")) {
            if (m_settingsSystem->SaveAllSettings()) {
                showPopup = false;
                ImGui::CloseCurrentPopup();
            }
        }

        if (m_settingsSystem->HasAnyUnsavedChanges()) {
            ImGui::SameLine();
            if (ImGui::Button("Cancel Changes")) {
                m_settingsSystem->RestoreAllFromBackups();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset to Defaults")) {
                m_settingsSystem->ResetAllToDefaults();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            HandlePopupClose();
        }
    }

    void SettingsView::RenderUnsavedChangesDialog() {
        if (showUnsavedChangesDialog) {
            ImGui::OpenPopup("Unsaved Changes");
        }

        if (ImGui::BeginPopupModal("Unsaved Changes", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("You have unsaved changes. What would you like to do?");
            ImGui::Separator();

            if (ImGui::Button("Apply and Close", ImVec2(120, 0))) {
                if (m_settingsSystem && m_settingsSystem->SaveAllSettings()) {
                    showUnsavedChangesDialog = false;
                    pendingClose = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();

            if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
                if (m_settingsSystem) {
                    m_settingsSystem->RestoreAllFromBackups();
                }
                showUnsavedChangesDialog = false;
                pendingClose = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                showUnsavedChangesDialog = false;
                pendingClose = false;
                showPopup = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

}