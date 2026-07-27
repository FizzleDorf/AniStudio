#include "ImGuiStyleSettingsTab.hpp"
#include <imgui_internal.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace ECS {

    ImGuiStyleSettingsTab::ImGuiStyleSettingsTab(ImGuiStyleSettingsComponent& comp) : m_comp(comp) {}

    bool ImGuiStyleSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void ImGuiStyleSettingsTab::Render() {
        m_comp.EnsureInitialized();
        ImGuiStyle& style = m_comp.currentStyle;

        if (ImGui::BeginChild("ImGuiStyleSettings", ImVec2(0, 0), false)) {
            if (FilterPass("Style Presets")) RenderStylePresets();
            if (FilterPass("Size Settings")) RenderSizeSettings();
            if (FilterPass("Border Settings")) RenderBorderSettings();
            if (FilterPass("Rounding Settings")) RenderRoundingSettings();
            if (FilterPass("Color Settings")) RenderColorSettings();
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void ImGuiStyleSettingsTab::RenderStylePresets() {
        const char* preview = m_comp.displayNames[m_comp.selectedStyleIndex].c_str();
        if (ImGui::BeginCombo("Theme", preview)) {
            for (int i = 0; i < (int)m_comp.displayNames.size(); ++i) {
                bool isSelected = (i == m_comp.selectedStyleIndex);
                if (ImGui::Selectable(m_comp.displayNames[i].c_str(), isSelected)) {
                    m_comp.selectedStyleIndex = i;
                    if (i < 4) {
                        m_comp.ApplyBuiltInStyle(i);
                    }
                    else {
                        int fileIdx = i - 4;
                        if (fileIdx < (int)m_comp.availableStyles.size()) {
                            m_comp.ApplyFileStyle(m_comp.availableStyles[fileIdx].path);
                        }
                    }
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh List")) {
            m_comp.ScanStylesDirectory();
            m_comp.RebuildDisplayList();
            if (m_comp.selectedStyleIndex >= (int)m_comp.displayNames.size())
                m_comp.selectedStyleIndex = 0;
        }
        ImGui::Separator();
        ImGui::Text("Save current style as:");
        ImGui::InputText("Filename", m_comp.saveAsFilename, IM_ARRAYSIZE(m_comp.saveAsFilename));
        ImGui::SameLine();
        if (ImGui::Button("Save As")) {
            std::string filename(m_comp.saveAsFilename);
            if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".json")
                filename += ".json";
            std::string fullPath = m_comp.GetStylesDirectory() + filename;
            std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
            m_comp.SaveStyleToFile(m_comp.currentStyle, fullPath);
            m_comp.ScanStylesDirectory();
            m_comp.RebuildDisplayList();
            for (int i = 0; i < (int)m_comp.availableStyles.size(); ++i) {
                if (m_comp.availableStyles[i].name == filename) {
                    m_comp.selectedStyleIndex = 4 + i;
                    break;
                }
            }
        }
        ImGui::Separator();
    }

    void ImGuiStyleSettingsTab::RenderSizeSettings() {
        ImGuiStyle& style = m_comp.currentStyle;
        if (ImGui::SliderFloat2("Window Padding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat2("Frame Padding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat2("Item Spacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat2("Item Inner Spacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Indent Spacing", &style.IndentSpacing, 0.0f, 30.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Scrollbar Size", &style.ScrollbarSize, 1.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Grab Min Size", &style.GrabMinSize, 1.0f, 20.0f, "%.1f")) m_comp.hasChanges = true;
    }

    void ImGuiStyleSettingsTab::RenderBorderSettings() {
        ImGuiStyle& style = m_comp.currentStyle;
        if (ImGui::SliderFloat("Window Border Size", &style.WindowBorderSize, 0.0f, 1.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Child Border Size", &style.ChildBorderSize, 0.0f, 1.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Popup Border Size", &style.PopupBorderSize, 0.0f, 1.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Frame Border Size", &style.FrameBorderSize, 0.0f, 1.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Tab Border Size", &style.TabBorderSize, 0.0f, 1.0f, "%.1f")) m_comp.hasChanges = true;
    }

    void ImGuiStyleSettingsTab::RenderRoundingSettings() {
        ImGuiStyle& style = m_comp.currentStyle;
        if (ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Child Rounding", &style.ChildRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Popup Rounding", &style.PopupRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Scrollbar Rounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Grab Rounding", &style.GrabRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
        if (ImGui::SliderFloat("Tab Rounding", &style.TabRounding, 0.0f, 12.0f, "%.1f")) m_comp.hasChanges = true;
    }

    void ImGuiStyleSettingsTab::RenderColorSettings() {
        ImGuiStyle& style = m_comp.currentStyle;
        static ImGuiTextFilter colorFilter;
        colorFilter.Draw("Filter Colors", ImGui::GetFontSize() * 16);
        static ImGuiColorEditFlags alphaFlags = ImGuiColorEditFlags_AlphaPreview;
        if (ImGui::RadioButton("Opaque", alphaFlags == ImGuiColorEditFlags_None)) alphaFlags = ImGuiColorEditFlags_None;
        ImGui::SameLine();
        if (ImGui::RadioButton("Alpha", alphaFlags == ImGuiColorEditFlags_AlphaPreview)) alphaFlags = ImGuiColorEditFlags_AlphaPreview;
        ImGui::SameLine();
        if (ImGui::RadioButton("Both", alphaFlags == ImGuiColorEditFlags_AlphaPreviewHalf)) alphaFlags = ImGuiColorEditFlags_AlphaPreviewHalf;
        if (ImGui::BeginChild("colors", ImVec2(0, 300), true)) {
            ImGui::PushItemWidth(-160);
            for (int i = 0; i < ImGuiCol_COUNT; i++) {
                const char* name = ImGui::GetStyleColorName(i);
                if (!colorFilter.PassFilter(name)) continue;
                ImGui::PushID(i);
                if (ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alphaFlags)) {
                    m_comp.hasChanges = true;
                }
                if (memcmp(&style.Colors[i], &m_comp.backupStyle.Colors[i], sizeof(ImVec4)) != 0) {
                    ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                    if (ImGui::Button("Revert")) {
                        style.Colors[i] = m_comp.backupStyle.Colors[i];
                        m_comp.hasChanges = true;
                    }
                }
                ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
                ImGui::TextUnformatted(name);
                ImGui::PopID();
            }
            ImGui::PopItemWidth();
        }
        ImGui::EndChild();
    }

    void ImGuiStyleSettingsTab::RenderActionButtons() {
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