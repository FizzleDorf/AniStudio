#include "GeneralSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>

namespace ECS {

    GeneralSettingsTab::GeneralSettingsTab(GeneralSettingsComponent& comp) : m_comp(comp) {}

    bool GeneralSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void GeneralSettingsTab::Render() {
        if (ImGui::BeginChild("GeneralSettings", ImVec2(0, 0), false)) {
            if (FilterPass("Startup")) RenderStartupSettings();
            if (FilterPass("Auto-Save")) RenderAutoSaveSettings();
            if (FilterPass("Confirmation")) RenderConfirmationSettings();
            if (FilterPass("Performance")) RenderPerformanceSettings();
            if (FilterPass("Logging")) RenderLoggingSettings();
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void GeneralSettingsTab::RenderStartupSettings() {
        if (ImGui::Checkbox("Show Startup Screen", &m_comp.showStartupScreen)) {}
        if (ImGui::Checkbox("Load Last Project on Startup", &m_comp.loadLastProject)) {}
        ImGui::Separator();
    }

    void GeneralSettingsTab::RenderAutoSaveSettings() {
        if (ImGui::Checkbox("Auto-Save Projects", &m_comp.autoSaveProjects)) {}
        if (m_comp.autoSaveProjects) {
            if (ImGui::SliderInt("Auto-Save Interval (minutes)", &m_comp.autoSaveIntervalMinutes, 1, 60)) {}
        }
        ImGui::Separator();
    }

    void GeneralSettingsTab::RenderConfirmationSettings() {
        if (ImGui::Checkbox("Confirm Before Exit", &m_comp.confirmBeforeExit)) {}
        if (ImGui::Checkbox("Confirm Before Delete Assets", &m_comp.confirmBeforeDeleteAssets)) {}
        if (ImGui::Checkbox("Confirm Before Overwrite Files", &m_comp.confirmBeforeOverwriteFiles)) {}
        ImGui::Separator();
    }

    void GeneralSettingsTab::RenderPerformanceSettings() {
        if (ImGui::SliderInt("Max Recent Projects", &m_comp.maxRecentProjects, 5, 50)) {}
        if (ImGui::SliderInt("Max Undo Levels", &m_comp.maxUndoLevels, 10, 1000)) {}
        if (ImGui::Checkbox("Enable Hardware Acceleration", &m_comp.enableHardwareAcceleration)) {}
        ImGui::Separator();
    }

    void GeneralSettingsTab::RenderLoggingSettings() {
        if (ImGui::Checkbox("Enable Logging", &m_comp.enableLogging)) {}
        if (m_comp.enableLogging) {
            const char* logLevels[] = { "Error", "Warning", "Info", "Debug" };
            if (ImGui::Combo("Log Level", &m_comp.logLevel, logLevels, 4)) {}
            if (ImGui::Checkbox("Log to File", &m_comp.logToFile)) {}
            if (m_comp.logToFile) {
                if (ImGui::SliderInt("Max Log File Size (MB)", &m_comp.maxLogFileSize, 1, 100)) {}
            }
        }
        ImGui::Separator();
    }

    void GeneralSettingsTab::RenderActionButtons() {
        if (ImGui::Button("Save Settings")) SaveSettings();
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (m_comp.HasUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
    }

}