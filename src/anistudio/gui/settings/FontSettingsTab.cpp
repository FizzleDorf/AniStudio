#include "FontSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>
#include <iostream>

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
            if (FilterPass("Icon Font")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Icon Font")) RenderIconFontFamily();
            }
            if (FilterPass("Global Scale")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Global Scale")) RenderScale();
            }
            RenderActionButtons();

            // Debug section
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Debug Info:");
            ImGui::Text("Selected Icon Font: %s", m_comp.selectedIconFontName.c_str());
            ImGui::Text("Icon Font Path: %s", m_comp.pendingIconFontPath.c_str());
            ImGui::Text("Icon Font Exists: %s",
                std::filesystem::exists(m_comp.pendingIconFontPath) ? "Yes" : "No");

            // Test the icon rendering
            ImGui::Text("Icon Test: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                "%s %s %s %s %s",
                Icon::Play().c_str(),
                Icon::Stop().c_str(),
                Icon::Save().c_str(),
                Icon::Refresh().c_str(),
                Icon::Trash().c_str());
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

    void FontSettingsTab::RenderIconFontFamily() {
        ImGui::TextUnformatted("Icon Font");

        // Check if we have any icon fonts
        if (m_comp.availableIconFonts.empty()) {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
                "No icon fonts found in assets/icon_fonts/");
            ImGui::TextDisabled("Place your Font Awesome .ttf or .otf file in:");
            ImGui::TextDisabled("  assets/icon_fonts/fa-solid-900.otf");
            ImGui::TextDisabled("  assets/icon_fonts/fa-solid-900.ttf");
            return;
        }

        if (ImGui::BeginCombo("##IconFontFamily", m_comp.selectedIconFontName.c_str())) {
            for (const auto& entry : m_comp.availableIconFonts) {
                bool isSelected = (entry.name == m_comp.selectedIconFontName);
                if (ImGui::Selectable(entry.name.c_str(), isSelected)) {
                    if (m_comp.selectedIconFontName != entry.name) {
                        m_comp.selectedIconFontName = entry.name;
                        m_comp.ApplyFont();
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Icon fonts should contain glyphs in the Unicode Private Use Area (PUA)");
            ImGui::Text("Common ranges: f000-f8ff for Font Awesome");
            ImGui::EndTooltip();
        }

        // Show the full path of the selected font
        std::string iconPath;
        for (const auto& entry : m_comp.availableIconFonts) {
            if (entry.name == m_comp.selectedIconFontName) {
                iconPath = entry.path;
                break;
            }
        }
        if (!iconPath.empty()) {
            ImGui::TextDisabled("Path: %s", iconPath.c_str());
        }

        ImGui::Text("Preview: ");
        ImGui::SameLine();
        ImGui::Text("%s %s %s %s %s",
            Icon::Play().c_str(),
            Icon::Stop().c_str(),
            Icon::Save().c_str(),
            Icon::Refresh().c_str(),
            Icon::Trash().c_str());
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