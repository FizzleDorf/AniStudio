#include "TextEditorSettingsTab.hpp"
#include <imgui.h>
#include <algorithm>
#include "FileDialogUtil.hpp"

namespace ECS {

    TextEditorSettingsTab::TextEditorSettingsTab(TextEditorSettingsComponent& comp, FontSettingsComponent& fontComp)
        : m_comp(comp), m_fontComp(fontComp) {
    }

    bool TextEditorSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void TextEditorSettingsTab::Render() {
        if (ImGui::BeginChild("TextEditorSettings", ImVec2(0, 0), false)) {
            if (FilterPass("General")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("General")) RenderGeneralSettings();
            }
            if (FilterPass("Font")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Font")) RenderFontSettings();
            }
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void TextEditorSettingsTab::RenderGeneralSettings() {
        ImGui::TextUnformatted("Default Editor Settings");

        bool ln = m_comp.showLineNumbers;
        if (ImGui::Checkbox("Line Numbers (default)", &ln)) {
            m_comp.showLineNumbers = ln;
            m_comp.m_hasChanges = true;
        }
        ImGui::SameLine();
        bool ww = m_comp.wordWrap;
        if (ImGui::Checkbox("Word Wrap (default)", &ww)) {
            m_comp.wordWrap = ww;
            m_comp.m_hasChanges = true;
        }

        bool ws = m_comp.showWhitespace;
        if (ImGui::Checkbox("Show Whitespace (default)", &ws)) {
            m_comp.showWhitespace = ws;
            m_comp.m_hasChanges = true;
        }
        ImGui::SameLine();
        bool ai = m_comp.autoIndent;
        if (ImGui::Checkbox("Auto Indent (default)", &ai)) {
            m_comp.autoIndent = ai;
            m_comp.m_hasChanges = true;
        }

        bool lf = m_comp.lineFolding;
        if (ImGui::Checkbox("Line Folding (default)", &lf)) {
            m_comp.lineFolding = lf;
            m_comp.m_hasChanges = true;
        }

        int ts = m_comp.tabSize;
        if (ImGui::SliderInt("Tab Size (default)", &ts, 1, 8)) {
            m_comp.tabSize = ts;
            m_comp.m_hasChanges = true;
        }

        const char* languages[] = { "Plain Text", "Python", "C++", "C", "C#", "GLSL", "HLSL", "JSON", "Markdown", "SQL", "Lua", "AngelScript" };
        int currentIndex = 0;
        for (int i = 0; i < IM_ARRAYSIZE(languages); ++i) {
            if (m_comp.defaultLanguage == languages[i]) {
                currentIndex = i;
                break;
            }
        }
        if (ImGui::Combo("Default Language", &currentIndex, languages, IM_ARRAYSIZE(languages))) {
            m_comp.defaultLanguage = languages[currentIndex];
            m_comp.m_hasChanges = true;
        }

        ImGui::Text("Default Autocomplete File");
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            std::string outPath;
            std::string filter = "Keyword Files{.txt},All Files{.*}";
            if (FileDialog::OpenFile("Select Autocomplete Keywords File", filter, outPath, ".")) {
                m_comp.autocompleteFile = outPath;
                m_comp.m_hasChanges = true;
            }
        }
        ImGui::SameLine();
        if (!m_comp.autocompleteFile.empty()) {
            ImGui::Text("%s", m_comp.autocompleteFile.c_str());
        }
        else {
            ImGui::TextDisabled("(none selected)");
        }
    }

    void TextEditorSettingsTab::RenderFontSettings() {
        ImGui::TextUnformatted("Editor Font");

        m_fontComp.EnsureInitialized();

        bool useCustom = m_comp.useCustomFont;
        if (ImGui::Checkbox("Use Custom Font", &useCustom)) {
            m_comp.useCustomFont = useCustom;
            m_comp.m_hasChanges = true;
        }

        if (m_comp.useCustomFont) {
            const std::string currentFont = m_comp.editorFontName.empty() ? "Default (Global)" : m_comp.editorFontName;
            if (ImGui::BeginCombo("##EditorFont", currentFont.c_str())) {
                if (ImGui::Selectable("Default (Global)", m_comp.editorFontName.empty())) {
                    m_comp.editorFontName = "";
                    m_comp.m_hasChanges = true;
                }
                for (const auto& entry : m_fontComp.availableFonts) {
                    bool selected = (entry.name == m_comp.editorFontName);
                    if (ImGui::Selectable(entry.name.c_str(), selected)) {
                        m_comp.editorFontName = entry.name;
                        m_comp.m_hasChanges = true;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh Fonts")) {
                m_fontComp.RefreshFontList();
                m_fontComp.ApplyFont();
            }
        }
    }

    void TextEditorSettingsTab::RenderActionButtons() {
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