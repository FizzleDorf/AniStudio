#include "FontSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>

namespace ECS {

    FontSettingsTab::FontSettingsTab(FontSettingsComponent& comp) : m_comp(comp) {}

    bool FontSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void FontSettingsTab::Render() {
        m_comp.EnsureInitialized();
        if (ImGui::BeginChild("FontSettings", ImVec2(0, 0), false)) {
            if (FilterPass("Font Family")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Font Family")) RenderFontFamily();
            }
            if (FilterPass("Global Scale")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Global Scale")) RenderScale();
            }
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void FontSettingsTab::RenderFontFamily() {
        ImGui::TextUnformatted("Font Family");
        if (ImGui::BeginCombo("##FontFamily", m_comp.selectedFontName.c_str())) {
            for (const auto& entry : m_comp.availableFonts) {
                bool isSelected = (entry.name == m_comp.selectedFontName);
                if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                    if (m_comp.selectedFontName != entry.name) {
                        m_comp.selectedFontName = entry.name;
                        m_comp.ApplyFont();
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    void FontSettingsTab::RenderScale() {
        ImGui::TextUnformatted("Global Font Scale");
        float scaleValue = m_comp.m_globalFontScale;
        if (ImGui::SliderFloat("##GlobalFontScale", &scaleValue, 0.5f, 2.5f, "%.2fx")) {
            m_comp.m_globalFontScale = scaleValue;
            if (m_comp.imguiContext) {
                ImGui::SetCurrentContext(m_comp.imguiContext);
                ImGui::GetIO().FontGlobalScale = m_comp.m_globalFontScale;
            }
            m_comp.hasChanges = true;
        }
    }

    void FontSettingsTab::RenderActionButtons() {
        if (ImGui::Button("Apply Font")) { m_comp.ApplyFont(); }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Font List")) { m_comp.RefreshFontList(); }
        ImGui::SameLine();
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