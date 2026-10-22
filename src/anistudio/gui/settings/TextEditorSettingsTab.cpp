#include "TextEditorSettingsTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "FontSettingsTab.hpp"
#include "UISchema.hpp"
#include <imgui.h>

namespace ECS {

    TextEditorSettingsTab::TextEditorSettingsTab(SettingsSystem& sys,
        TextEditorSettingsComponent& comp,
        FontSettingsComponent& fontComp)
        : m_settingsSystem(sys), m_comp(comp), m_fontComp(fontComp) {
    }

    void TextEditorSettingsTab::Render() {
        if (ImGui::BeginChild("TextEditorSettings", ImVec2(0, 0), false)) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("General")) {
                auto props = m_comp.GetPropertyMap();
                auto* filePathComp = m_settingsSystem.GetFilePathComponent();
                const std::unordered_map<std::string, std::string>* pathMap =
                    filePathComp ? &filePathComp->GetCurrentMap() : nullptr;

                UISchema::RenderSchema(
                    m_comp.GetSchema(),
                    props,
                    nullptr,
                    m_comp.GetCompName(),
                    (int)m_settingsSystem.GetSettingsEntity(),
                    pathMap);
            }

            ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
            if (ImGui::CollapsingHeader("Font Override")) RenderFontOverride();
        }
        ImGui::EndChild();
    }

    void TextEditorSettingsTab::RenderFontOverride() {
        if (!m_comp.useCustomFont) {
            ImGui::TextDisabled("Enable \"Use Custom Font\" above to select a per-editor font.");
            return;
        }

        const std::string currentFont =
            m_comp.editorFontName.empty() ? "Default (Global)" : m_comp.editorFontName;
        if (ImGui::BeginCombo("##EditorFont", currentFont.c_str())) {
            if (ImGui::Selectable("Default (Global)", m_comp.editorFontName.empty())) {
                m_comp.editorFontName.clear();
            }
            for (const auto& entry : m_fontComp.availableFonts) {
                bool selected = (entry.name == m_comp.editorFontName);
                if (ImGui::Selectable(entry.name.c_str(), selected)) {
                    m_comp.editorFontName = entry.name;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh Fonts")) {
            FontSettingsTab::ScanFontsDirectories(m_fontComp);
        }
    }

}