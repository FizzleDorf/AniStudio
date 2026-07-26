#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <filesystem>
#include "UISchemaUtils.hpp"
#include "UISchemaContext.hpp"
#include <TextEditor.h>
#include "TextEditorUtil.hpp"
#include "TextEditorFontUtil.hpp"
#include "FileDialogUtil.hpp"
#include "TextEditorSettingsComponent.hpp"

namespace UISchema {

    static const std::string DEFAULT_STRING_WIDGET = "input_text";

    struct TextEditorConfig {
        bool showLineNumbers = false;
        bool wordWrap = true;
        bool readOnly = false;
        bool showWhitespace = false;
        bool enableSyntaxHighlighting = true;
        bool autoIndent = true;
        bool showSearch = false;
        std::string theme = "dark";
    };

    struct EditorInstanceData {
        TextEditorConfig config;
        const TextEditor::Language* language = nullptr;
        bool autocompleteSetup = false;
        std::string filePath;
        float height = 150.0f;
    };

    class StringWidgets {
    public:
        static void SetSettingsComponent(ECS::TextEditorSettingsComponent* comp) { s_settings = comp; }

        static std::unordered_map<std::string, std::shared_ptr<TextEditor>>& GetEditorMap() {
            static std::unordered_map<std::string, std::shared_ptr<TextEditor>> editorMap;
            return editorMap;
        }

        static std::unordered_map<std::string, EditorInstanceData>& GetInstanceData() {
            static std::unordered_map<std::string, EditorInstanceData> instanceData;
            return instanceData;
        }

        static std::unordered_map<std::string, std::string>& GetLastKnownValues() {
            static std::unordered_map<std::string, std::string> lastKnownValues;
            return lastKnownValues;
        }

        static std::shared_ptr<TextEditor> GetOrCreateEditor(std::string* value, const std::string& uniqueLabel) {
            auto& editorMap = GetEditorMap();
            auto& instanceData = GetInstanceData();
            auto& lastKnownValues = GetLastKnownValues();

            std::string uniqueKey = uniqueLabel;
            if (uniqueKey.substr(0, 2) == "##") {
                uniqueKey = uniqueKey.substr(2) + "_text_editor";
            }

            auto it = editorMap.find(uniqueKey);
            if (it == editorMap.end()) {
                auto editor = std::make_shared<TextEditor>();
                editor->SetLanguage(nullptr);
                editor->SetText(*value);

                TextEditorConfig defaultConfig;
                defaultConfig.showLineNumbers = false;
                defaultConfig.wordWrap = true;

                if (s_settings) {
                    defaultConfig.showLineNumbers = s_settings->showLineNumbers;
                    defaultConfig.wordWrap = s_settings->wordWrap;
                    defaultConfig.showWhitespace = s_settings->showWhitespace;
                    defaultConfig.autoIndent = s_settings->autoIndent;
                    ApplyConfig(editor, defaultConfig);
                    editor->SetLineFoldingEnabled(s_settings->lineFolding);
                    editor->SetTabSize(static_cast<size_t>(s_settings->tabSize));
                    auto lang = TextEditorUtil::getLanguageFromName(s_settings->defaultLanguage);
                    if (lang) editor->SetLanguage(lang);
                    if (!s_settings->autocompleteFile.empty()) {
                        TextEditorUtil::setupAutocomplete(editor.get(), s_settings->autocompleteFile);
                    }
                }
                else {
                    ApplyConfig(editor, defaultConfig);
                }

                editorMap[uniqueKey] = editor;
                instanceData[uniqueKey] = EditorInstanceData{};
                instanceData[uniqueKey].config = defaultConfig;
                instanceData[uniqueKey].height = 150.0f;
                lastKnownValues[uniqueKey] = *value;

                if (!s_settings) {
                    std::string keywordsFile = "python_keywords.txt";
                    TextEditorUtil::setupAutocomplete(editor.get(), keywordsFile);
                }

                instanceData[uniqueKey].autocompleteSetup = true;
                TextEditorUtil::updateEditorPalette(editor.get());

                return editor;
            }
            return it->second;
        }

        static void ApplyConfig(std::shared_ptr<TextEditor> editor, const TextEditorConfig& config) {
            if (!editor) return;
            editor->SetShowLineNumbersEnabled(config.showLineNumbers);
            editor->SetWordWrapEnabled(config.wordWrap);
            editor->SetReadOnlyEnabled(config.readOnly);
            editor->SetShowWhitespacesEnabled(config.showWhitespace);
            editor->SetAutoIndentEnabled(config.autoIndent);
        }

        static TextEditorConfig ParseSchemaOptions(const nlohmann::json& options) {
            TextEditorConfig config;
            config.showLineNumbers = GetSchemaValue<bool>(options, "showLineNumbers", false);
            config.wordWrap = GetSchemaValue<bool>(options, "wordWrap", true);
            config.readOnly = GetSchemaValue<bool>(options, "readOnly", config.readOnly);
            config.showWhitespace = GetSchemaValue<bool>(options, "showWhitespace", config.showWhitespace);
            config.enableSyntaxHighlighting = GetSchemaValue<bool>(options, "enableSyntaxHighlighting", config.enableSyntaxHighlighting);
            config.autoIndent = GetSchemaValue<bool>(options, "autoIndent", config.autoIndent);
            config.showSearch = GetSchemaValue<bool>(options, "showSearch", config.showSearch);
            return config;
        }

        static std::string GetUniqueBaseId(const std::string& label) {
            if (label.substr(0, 2) == "##") {
                return label.substr(2);
            }
            return label;
        }

        static bool RenderTextEditor(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            auto editor = GetOrCreateEditor(value, label);
            if (!editor) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to create text editor");
                return false;
            }

            TextEditorUtil::updateEditorPalette(editor.get());

            auto& instanceData = GetInstanceData();
            auto& lastKnownValues = GetLastKnownValues();

            std::string uniqueKey = GetUniqueBaseId(label) + "_text_editor";
            auto& data = instanceData[uniqueKey];

            if (data.config.showLineNumbers == false && data.config.wordWrap == true) {
                data.config = ParseSchemaOptions(options);
                ApplyConfig(editor, data.config);
            }

            if (schema.contains("ui:displayName") && schema["ui:displayName"].is_string()) {
                std::string displayName = schema["ui:displayName"].get<std::string>();
                ImGui::Text("%s", displayName.c_str());
            }

            auto lastValueIt = lastKnownValues.find(uniqueKey);
            if (lastValueIt != lastKnownValues.end() && lastValueIt->second != *value) {
                editor->SetText(*value);
                lastKnownValues[uniqueKey] = *value;
            }

            bool modified = false;

            ImGui::PushID(uniqueKey.c_str());

            if (ImGui::CollapsingHeader("Editor Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Button("Undo")) {
                    if (editor->CanUndo()) editor->Undo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Redo")) {
                    if (editor->CanRedo()) editor->Redo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Find")) {
                    editor->OpenFindReplaceWindow();
                }
                ImGui::SameLine();
                if (ImGui::Button("Load...")) {
                    std::string outPath;
                    std::string filter = "Text Files{.txt},All Files{.*}";
                    if (FileDialog::OpenFile("Open Text File", filter, outPath, ".")) {
                        std::ifstream file(outPath);
                        if (file.is_open()) {
                            std::stringstream buffer;
                            buffer << file.rdbuf();
                            editor->SetText(buffer.str());
                            data.filePath = outPath;
                            lastKnownValues[uniqueKey] = editor->GetText();
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Save...")) {
                    std::string outPath;
                    std::string defaultName = data.filePath.empty() ? "untitled.txt" : std::filesystem::path(data.filePath).filename().string();
                    std::string filter = "Text Files{.txt},All Files{.*}";
                    if (FileDialog::SaveFile("Save Text File", filter, defaultName, outPath, ".")) {
                        std::ofstream file(outPath);
                        if (file.is_open()) {
                            file << editor->GetText();
                            data.filePath = outPath;
                        }
                    }
                }

                ImGui::Separator();

                bool lineNumbers = editor->IsShowLineNumbersEnabled();
                if (ImGui::Checkbox("Line Numbers", &lineNumbers)) {
                    editor->SetShowLineNumbersEnabled(lineNumbers);
                    data.config.showLineNumbers = lineNumbers;
                }
                ImGui::SameLine();
                bool wordWrap = editor->IsWordWrapEnabled();
                if (ImGui::Checkbox("Word Wrap", &wordWrap)) {
                    editor->SetWordWrapEnabled(wordWrap);
                    data.config.wordWrap = wordWrap;
                }

                ImGui::Separator();

                const char* langNames[] = { "Plain Text", "Python", "C++", "C", "C#", "GLSL", "HLSL", "JSON", "Markdown", "SQL", "Lua", "AngelScript" };
                const TextEditor::Language* langList[] = {
                    nullptr,
                    TextEditor::Language::Python(),
                    TextEditor::Language::Cpp(),
                    TextEditor::Language::C(),
                    TextEditor::Language::Cs(),
                    TextEditor::Language::Glsl(),
                    TextEditor::Language::Hlsl(),
                    TextEditor::Language::Json(),
                    TextEditor::Language::Markdown(),
                    TextEditor::Language::Sql(),
                    TextEditor::Language::Lua(),
                    TextEditor::Language::AngelScript()
                };
                int currentLangIndex = 0;
                for (int i = 0; i < IM_ARRAYSIZE(langList); ++i) {
                    if (langList[i] == editor->GetLanguage()) {
                        currentLangIndex = i;
                        break;
                    }
                }
                if (ImGui::Combo("Language", &currentLangIndex, langNames, IM_ARRAYSIZE(langNames))) {
                    auto newLang = langList[currentLangIndex];
                    editor->SetLanguage(newLang);
                    data.language = newLang;
                    if (newLang == nullptr) {
                        TextEditorUtil::clearAutocomplete(editor.get());
                    }
                    else {
                        TextEditorUtil::setupAutocomplete(editor.get(), "python_keywords.txt");
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Load Autocomplete...")) {
                    std::string outPath;
                    std::string filter = "Keyword Files{.txt},All Files{.*}";
                    if (FileDialog::OpenFile("Load Autocomplete Keywords", filter, outPath, ".")) {
                        TextEditorUtil::setupAutocomplete(editor.get(), outPath);
                    }
                }
            }

            ImGui::Separator();

            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            if (contentSize.x < 100) contentSize.x = 100;
            if (contentSize.y < 100) contentSize.y = 100;

            if (data.height < 50.0f) data.height = 150.0f;

            std::string childId = "##editor_child_" + uniqueKey;
            ImVec2 childSize(contentSize.x, data.height);
            if (ImGui::BeginChild(childId.c_str(), childSize, ImGuiChildFlags_ResizeY)) {
                TextEditorUtil::pushEditorFont();
                editor->Render(label.c_str(), ImGui::GetContentRegionAvail());
                TextEditorUtil::popEditorFont();
            }
            ImGui::EndChild();

            ImVec2 newSize = ImGui::GetItemRectSize();
            if (newSize.y > 10.0f) {
                data.height = newSize.y;
            }

            ImGui::PopID();

            std::string currentText = editor->GetText();
            if (currentText != *value) {
                *value = currentText;
                lastKnownValues[uniqueKey] = *value;
                modified = true;
            }

            return modified;
        }

        static bool RenderZepEditor(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            return RenderTextEditor(label, value, options, schema);
        }
        static bool RenderZepEditorVim(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            return RenderTextEditor(label, value, options, schema);
        }
        static bool RenderZepEditorStandard(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            return RenderTextEditor(label, value, options, schema);
        }

        static ImGuiInputTextFlags GetInputTextFlags(const nlohmann::json& schema) {
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
            if (schema.contains("ui:flags")) {
                if (schema["ui:flags"].is_number()) {
                    return static_cast<ImGuiInputTextFlags>(schema["ui:flags"].get<int>());
                }
                else if (schema["ui:flags"].is_array()) {
                    for (const auto& flag : schema["ui:flags"]) {
                        if (flag.is_number()) {
                            flags |= static_cast<ImGuiInputTextFlags>(flag.get<int>());
                        }
                    }
                }
            }
            if (GetSchemaValue<bool>(schema, "readOnly", false)) {
                flags |= ImGuiInputTextFlags_ReadOnly;
            }
            if (GetSchemaValue<bool>(schema, "password", false)) {
                flags |= ImGuiInputTextFlags_Password;
            }
            if (GetSchemaValue<bool>(schema, "ui:options.allowTabInput", false)) {
                flags |= ImGuiInputTextFlags_AllowTabInput;
            }
            return flags;
        }

        static bool RenderInputText(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            ImGuiInputTextFlags flags = GetInputTextFlags(options);
            size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 1024);
            if (bufferSize < 1) bufferSize = 1;
            std::vector<char> buffer(bufferSize);
            strncpy(buffer.data(), value->c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            bool changed = ImGui::InputText(label.c_str(), buffer.data(), bufferSize, flags);
            if (changed) {
                *value = buffer.data();
            }
            return changed;
        }

        static bool RenderTextArea(const std::string& label, std::string* value, const nlohmann::json& options, const nlohmann::json& schema) {
            ImGuiInputTextFlags flags = GetInputTextFlags(options);
            size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 4096);
            if (bufferSize < 1) bufferSize = 1;
            std::vector<char> buffer(bufferSize);
            strncpy(buffer.data(), value->c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';

            if (schema.contains("ui:displayName") && schema["ui:displayName"].is_string()) {
                std::string displayName = schema["ui:displayName"].get<std::string>();
                ImGui::Text("%s", displayName.c_str());
            }

            float minHeight = ImGui::GetTextLineHeight() * 3.0f + ImGui::GetStyle().FramePadding.y * 2.0f;
            int rows = GetSchemaValue<int>(options, "rows", 5);
            float initialHeight = ImGui::GetTextLineHeight() * rows + ImGui::GetStyle().FramePadding.y * 2.0f;
            std::string uniqueBaseId = GetUniqueBaseId(label);
            std::string childId = uniqueBaseId + "_textarea_child";

            if (ImGui::BeginChild(childId.c_str(), ImVec2(0, std::max(minHeight, initialHeight)), ImGuiChildFlags_ResizeY)) {
                std::string textareaId = "##" + uniqueBaseId + "_textarea_input";
                bool changed = ImGui::InputTextMultiline(
                    textareaId.c_str(),
                    buffer.data(),
                    bufferSize,
                    ImVec2(-1.0f, -1.0f),
                    flags | ImGuiInputTextFlags_AllowTabInput
                );
                if (changed) {
                    *value = buffer.data();
                }
                ImGui::EndChild();
                return changed;
            }
            ImGui::EndChild();
            return false;
        }

        static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema, const UIRenderContext& context) {
            nlohmann::json options = {};
            if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
                options = schema["ui:options"];
            }

            if (widgetType == "input_text") {
                return RenderInputText(label, value, options, schema);
            }
            else if (widgetType == "text_editor" || widgetType == "text_editor_vim" || widgetType == "text_editor_standard") {
                return RenderTextEditor(label, value, options, schema);
            }
            else if (widgetType == "text_area") {
                return RenderTextArea(label, value, options, schema);
            }
            else {
                return RenderInputText(label, value, options, schema);
            }
        }

        static void UpdateEditorContent(std::string* value, const std::string& uniqueLabel) {
            auto& editorMap = GetEditorMap();
            auto& lastKnownValues = GetLastKnownValues();
            std::string uniqueKey = GetUniqueBaseId(uniqueLabel) + "_text_editor";
            auto it = editorMap.find(uniqueKey);
            if (it != editorMap.end()) {
                it->second->SetText(*value);
                lastKnownValues[uniqueKey] = *value;
            }
        }

        static void CleanupEditor(std::string* value, const std::string& uniqueLabel) {
            auto& editorMap = GetEditorMap();
            auto& instanceData = GetInstanceData();
            auto& lastKnownValues = GetLastKnownValues();
            std::string uniqueKey = GetUniqueBaseId(uniqueLabel) + "_text_editor";
            auto it = editorMap.find(uniqueKey);
            if (it != editorMap.end()) {
                TextEditorUtil::clearAutocomplete(it->second.get());
            }
            editorMap.erase(uniqueKey);
            instanceData.erase(uniqueKey);
            lastKnownValues.erase(uniqueKey);
        }

        static void Cleanup() {
            GetEditorMap().clear();
            GetInstanceData().clear();
            GetLastKnownValues().clear();
        }

    private:
        inline static ECS::TextEditorSettingsComponent* s_settings = nullptr;
    };

} // namespace UISchema