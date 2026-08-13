// SDCPPSettingsTab.cpp
#include "SDCPPSettingsTab.hpp"
#include "UISchema.hpp"
#include <imgui.h>

namespace ECS {

    SDCPPSettingsTab::SDCPPSettingsTab(SDCPPSettingsComponent& comp) : m_comp(comp) {}

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

            UISchema::RenderSchema(
                m_comp.schema,
                m_comp.GetPropertyMap(),
                nullptr,
                "SDCPP",
                0,
                nullptr
            );

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

} // namespace ECS