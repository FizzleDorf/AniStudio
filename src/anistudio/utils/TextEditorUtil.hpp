#pragma once

#include <TextEditor.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <functional>
#include <cstring>
#include <imgui.h>

namespace TextEditorUtil {

    using EditorPtr = TextEditor*;

    struct AutocompleteData {
        std::vector<std::string> keywords;
        bool loaded = false;
    };

    static std::unordered_map<EditorPtr, AutocompleteData> autocompleteData;

    static void loadKeywordsFromFile(EditorPtr editor, const std::string& filepath) {
        auto& data = autocompleteData[editor];
        data.keywords.clear();
        std::ifstream file(filepath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    data.keywords.push_back(line);
                }
            }
            data.loaded = true;
        }
    }

    static void setupAutocomplete(EditorPtr editor, const std::string& keywordsFile) {
        loadKeywordsFromFile(editor, keywordsFile);
        auto& data = autocompleteData[editor];
        if (!data.loaded) return;

        TextEditor::AutoCompleteConfig config;
        config.triggerOnTyping = true;
        config.triggerOnShortcut = true;
        config.autoInsertSingleSuggestions = false;
        config.suggestionWidth = 30;

        config.callback = [](TextEditor::AutoCompleteState& state) {
            auto it = autocompleteData.find(state.userData ? static_cast<EditorPtr>(state.userData) : nullptr);
            if (it != autocompleteData.end() && it->second.loaded) {
                state.suggestions = it->second.keywords;
            }
            };
        config.userData = editor;

        editor->SetAutoCompleteConfig(&config);
    }

    static void clearAutocomplete(EditorPtr editor) {
        autocompleteData.erase(editor);
        editor->SetAutoCompleteConfig(nullptr);
    }

    static void addErrorMarker(EditorPtr editor, size_t line, const std::string& tooltip) {
        ImU32 lineColor = IM_COL32(255, 0, 0, 128);
        ImU32 textColor = IM_COL32(255, 0, 0, 60);
        editor->AddMarker(line, lineColor, textColor, "Error", tooltip.c_str());
    }

    static void clearMarkers(EditorPtr editor) {
        editor->ClearMarkers();
    }

    static void configureEditor(EditorPtr editor, bool lineNumbers = true, bool wordWrap = false,
        bool readOnly = false, bool whitespace = false, bool autoIndent = true,
        bool folding = false, size_t tabSize = 4) {
        editor->SetShowLineNumbersEnabled(lineNumbers);
        editor->SetWordWrapEnabled(wordWrap);
        editor->SetReadOnlyEnabled(readOnly);
        editor->SetShowWhitespacesEnabled(whitespace);
        editor->SetAutoIndentEnabled(autoIndent);
        editor->SetLineFoldingEnabled(folding);
        editor->SetTabSize(tabSize);
    }

    static void setLanguage(EditorPtr editor, const TextEditor::Language* lang) {
        editor->SetLanguage(lang);
    }

    static void setupChangeCallback(EditorPtr editor, std::function<void()> callback, int delayMs = 500) {
        editor->SetChangeCallback(callback, delayMs);
    }

    static TextEditor::Palette buildPaletteFromImGuiStyle() {
        const auto& style = ImGui::GetStyle();
        TextEditor::Palette palette;

        ImVec4 bg = style.Colors[ImGuiCol_ChildBg];
        bool dark = (bg.x + bg.y + bg.z) / 3.0f < 0.5f;
        TextEditor::Palette defaultPalette = dark ? TextEditor::GetDarkPalette() : TextEditor::GetLightPalette();

        palette[static_cast<size_t>(TextEditor::Color::text)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
        palette[static_cast<size_t>(TextEditor::Color::background)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ChildBg]);
        palette[static_cast<size_t>(TextEditor::Color::selection)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Header]);
        palette[static_cast<size_t>(TextEditor::Color::lineNumber)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);
        palette[static_cast<size_t>(TextEditor::Color::currentLineNumber)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
        palette[static_cast<size_t>(TextEditor::Color::whitespace)] = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_TextDisabled]);

        ImVec4 text = style.Colors[ImGuiCol_Text];
        float luminance = 0.299f * text.x + 0.587f * text.y + 0.114f * text.z;
        ImVec4 cursorColor = (luminance > 0.5f) ? ImVec4(0, 0, 0, 1) : ImVec4(1, 1, 1, 1);
        palette[static_cast<size_t>(TextEditor::Color::cursor)] = ImGui::ColorConvertFloat4ToU32(cursorColor);

        palette[static_cast<size_t>(TextEditor::Color::keyword)] = defaultPalette[static_cast<size_t>(TextEditor::Color::keyword)];
        palette[static_cast<size_t>(TextEditor::Color::declaration)] = defaultPalette[static_cast<size_t>(TextEditor::Color::declaration)];
        palette[static_cast<size_t>(TextEditor::Color::number)] = defaultPalette[static_cast<size_t>(TextEditor::Color::number)];
        palette[static_cast<size_t>(TextEditor::Color::string)] = defaultPalette[static_cast<size_t>(TextEditor::Color::string)];
        palette[static_cast<size_t>(TextEditor::Color::punctuation)] = defaultPalette[static_cast<size_t>(TextEditor::Color::punctuation)];
        palette[static_cast<size_t>(TextEditor::Color::preprocessor)] = defaultPalette[static_cast<size_t>(TextEditor::Color::preprocessor)];
        palette[static_cast<size_t>(TextEditor::Color::identifier)] = defaultPalette[static_cast<size_t>(TextEditor::Color::identifier)];
        palette[static_cast<size_t>(TextEditor::Color::knownIdentifier)] = defaultPalette[static_cast<size_t>(TextEditor::Color::knownIdentifier)];
        palette[static_cast<size_t>(TextEditor::Color::comment)] = defaultPalette[static_cast<size_t>(TextEditor::Color::comment)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketBackground)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketBackground)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketActive)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketActive)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel1)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel1)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel2)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel2)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel3)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketLevel3)];
        palette[static_cast<size_t>(TextEditor::Color::matchingBracketError)] = defaultPalette[static_cast<size_t>(TextEditor::Color::matchingBracketError)];

        return palette;
    }

    static void updateEditorPalette(EditorPtr editor) {
        if (!editor) return;
        editor->SetPalette(buildPaletteFromImGuiStyle());
    }

    static const TextEditor::Language* getLanguageFromName(const std::string& name) {
        if (name == "Python") return TextEditor::Language::Python();
        if (name == "C++") return TextEditor::Language::Cpp();
        if (name == "C") return TextEditor::Language::C();
        if (name == "C#") return TextEditor::Language::Cs();
        if (name == "GLSL") return TextEditor::Language::Glsl();
        if (name == "HLSL") return TextEditor::Language::Hlsl();
        if (name == "JSON") return TextEditor::Language::Json();
        if (name == "Markdown") return TextEditor::Language::Markdown();
        if (name == "SQL") return TextEditor::Language::Sql();
        if (name == "Lua") return TextEditor::Language::Lua();
        if (name == "AngelScript") return TextEditor::Language::AngelScript();
        return nullptr;
    }

} // namespace TextEditorUtil