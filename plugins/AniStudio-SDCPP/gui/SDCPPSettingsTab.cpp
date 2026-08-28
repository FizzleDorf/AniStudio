// SDCPPSettingsTab.cpp
#include "SDCPPSettingsTab.hpp"
#include "UISchema.hpp"
#include <imgui.h>
#include <algorithm>

namespace ECS {

    SDCPPSettingsTab::SDCPPSettingsTab(SDCPPSettingsComponent& comp) : m_comp(comp) {}

    bool SDCPPSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void SDCPPSettingsTab::Render() {
        if (ImGui::BeginChild("SDCPPSettings", ImVec2(0, 0), false)) {
            ImGui::Text("Global SDCPP settings applied at context creation");
            ImGui::Separator();

            char filterBuf[64];
            strncpy(filterBuf, m_filter.c_str(), sizeof(filterBuf) - 1);
            filterBuf[sizeof(filterBuf) - 1] = '\0';
            if (ImGui::InputText("Filter", filterBuf, sizeof(filterBuf))) {
                m_filter = filterBuf;
            }

            if (FilterPass("SDCPP Options")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("SDCPP Options")) {
                    UISchema::RenderSchema(
                        m_comp.schema,
                        m_comp.GetPropertyMap(),
                        nullptr,
                        "SDCPP",
                        0,
                        nullptr
                    );
                }
            }

            ImGui::Separator();
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void SDCPPSettingsTab::RenderActionButtons() {
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