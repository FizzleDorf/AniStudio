#include "TextEditorView.hpp"
#include "FilePathSystem.hpp"
#include "TextEditorUtil.hpp"
#include "TextEditorFontUtil.hpp"
#include "SettingsSystem.hpp"
#include "TextEditorSettingsComponent.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <any>
#include <cstring>
#include "Events.hpp"

namespace GUI {

    void TextEditorView::Init() {
        auto settingsSystem = m_entityManager.GetSystem<ECS::SettingsSystem>();
        if (settingsSystem) {
            auto settingsEntity = settingsSystem->GetSettingsEntity();
            if (m_entityManager.IsEntityValid(settingsEntity)) {
                auto& settingsComp = m_entityManager.GetComponent<ECS::TextEditorSettingsComponent>(settingsEntity);
                textEditor->SetShowLineNumbersEnabled(true);
                textEditor->SetWordWrapEnabled(settingsComp.wordWrap);
                textEditor->SetShowWhitespacesEnabled(settingsComp.showWhitespace);
                textEditor->SetAutoIndentEnabled(settingsComp.autoIndent);
                textEditor->SetLineFoldingEnabled(settingsComp.lineFolding);
                textEditor->SetTabSize(static_cast<size_t>(settingsComp.tabSize));
                auto lang = TextEditorUtil::getLanguageFromName(settingsComp.defaultLanguage);
                if (lang) textEditor->SetLanguage(lang);
                if (!settingsComp.autocompleteFile.empty()) {
                    TextEditorUtil::setupAutocomplete(textEditor.get(), settingsComp.autocompleteFile);
                }
            }
        }

        if (!textEditor->GetLanguage()) {
            auto lang = TextEditor::Language::Python();
            textEditor->SetLanguage(lang);
        }

        if (!textEditor->IsAutoIndentEnabled()) {
            TextEditorUtil::configureEditor(textEditor.get(), textEditor->IsShowLineNumbersEnabled(),
                textEditor->IsWordWrapEnabled(), false,
                textEditor->IsShowWhitespacesEnabled(), true, false, 4);
        }
    }

    void TextEditorView::Update(const float deltaT) {
        // nothing
    }

    void TextEditorView::Render() {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

        if (ImGui::Begin(GetWindowTitle().c_str(), &windowOpen, ImGuiWindowFlags_MenuBar)) {
            if (!windowOpen) {
                std::unordered_map<std::string, std::any> eventData;
                eventData["workspaceID"] = GetID();
                eventData["viewTypeName"] = viewName;
                ANI::Events::Ref().QueueEventWithData("RemoveView", eventData);
                ImGui::End();
                return;
            }

            if (ImGui::BeginMenuBar()) {
                RenderFileMenu();
                RenderEditMenu();
                RenderViewMenu();
                RenderLanguageMenu();
                ImGui::EndMenuBar();
            }

            TextEditorUtil::updateEditorPalette(textEditor.get());

            ImVec2 contentSize = ImGui::GetContentRegionAvail();
            if (contentSize.x < 200) contentSize.x = 200;
            if (contentSize.y < 100) contentSize.y = 100;

            TextEditorUtil::pushEditorFont();
            textEditor->Render("##TextEditor", contentSize);
            TextEditorUtil::popEditorFont();

            RenderStatusBar();
        }
        ImGui::End();
    }

    std::string TextEditorView::GetText() const {
        return textEditor->GetText();
    }

    void TextEditorView::SetText(const std::string& text) {
        textEditor->SetText(text);
    }

    bool TextEditorView::LoadFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        SetText(buffer.str());
        currentFilePath = filePath;
        return true;
    }

    bool TextEditorView::SaveFile(const std::string& filePath) {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        file << GetText();
        file.close();
        currentFilePath = filePath;
        return true;
    }

    std::string TextEditorView::GetWindowTitle() const {
        std::string title = viewName;
        if (!currentFilePath.empty()) {
            std::filesystem::path path(currentFilePath);
            title += " - " + path.filename().string();
        }
        if (textEditor->CanUndo()) {
            title += " *";
        }
        return title + "##" + std::to_string(GetID());
    }

    void TextEditorView::RenderFileMenu() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                SetText("");
                currentFilePath.clear();
            }
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                OpenFileDialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (currentFilePath.empty()) {
                    SaveAsFileDialog();
                }
                else {
                    SaveFile(currentFilePath);
                }
            }
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                SaveAsFileDialog();
            }
            ImGui::EndMenu();
        }
    }

    void TextEditorView::RenderEditMenu() {
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, textEditor->CanUndo())) {
                textEditor->Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, textEditor->CanRedo())) {
                textEditor->Redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Find", "Ctrl+F")) {
                textEditor->OpenFindReplaceWindow();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear All")) {
                SetText("");
            }
            ImGui::EndMenu();
        }
    }

    void TextEditorView::RenderViewMenu() {
        if (ImGui::BeginMenu("View")) {
            bool showLineNumbers = textEditor->IsShowLineNumbersEnabled();
            if (ImGui::MenuItem("Line Numbers", nullptr, &showLineNumbers)) {
                textEditor->SetShowLineNumbersEnabled(showLineNumbers);
            }
            bool wordWrap = textEditor->IsWordWrapEnabled();
            if (ImGui::MenuItem("Word Wrap", nullptr, &wordWrap)) {
                textEditor->SetWordWrapEnabled(wordWrap);
            }
            bool showWhitespace = textEditor->IsShowWhitespacesEnabled();
            if (ImGui::MenuItem("Show Whitespace", nullptr, &showWhitespace)) {
                textEditor->SetShowWhitespacesEnabled(showWhitespace);
            }
            bool readOnly = textEditor->IsReadOnlyEnabled();
            if (ImGui::MenuItem("Read Only", nullptr, &readOnly)) {
                textEditor->SetReadOnlyEnabled(readOnly);
            }
            ImGui::Separator();
            bool folding = textEditor->IsLineFoldingEnabled();
            if (ImGui::MenuItem("Line Folding", nullptr, &folding)) {
                textEditor->SetLineFoldingEnabled(folding);
                if (folding) {
                    textEditor->UnfoldAll();
                }
            }
            ImGui::Separator();
            int tabSize = static_cast<int>(textEditor->GetTabSize());
            if (ImGui::SliderInt("Tab Size", &tabSize, 1, 8)) {
                textEditor->SetTabSize(static_cast<size_t>(tabSize));
            }
            ImGui::EndMenu();
        }
    }

    void TextEditorView::RenderLanguageMenu() {
        if (ImGui::BeginMenu("Language")) {
            const char* names[] = { "Plain Text", "Python", "C++", "C", "C#", "GLSL", "HLSL", "JSON", "Markdown", "SQL", "Lua", "AngelScript" };
            const TextEditor::Language* langs[] = {
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
            auto current = textEditor->GetLanguage();
            for (int i = 0; i < IM_ARRAYSIZE(names); ++i) {
                bool selected = (langs[i] == current);
                if (ImGui::MenuItem(names[i], nullptr, selected)) {
                    textEditor->SetLanguage(langs[i]);
                    if (langs[i] == nullptr) {
                        TextEditorUtil::clearAutocomplete(textEditor.get());
                    }
                    else {
                        TextEditorUtil::setupAutocomplete(textEditor.get(), "python_keywords.txt");
                    }
                }
            }
            ImGui::EndMenu();
        }
    }

    void TextEditorView::RenderStatusBar() {
        ImGui::Separator();
        auto cursorPos = textEditor->GetCurrentCursorPosition();
        auto lineCount = textEditor->GetLineCount();
        ImGui::Text("Line %zu, Col %zu   |   Lines: %zu   |   %s",
            cursorPos.line + 1, cursorPos.index + 1,
            lineCount,
            textEditor->CanUndo() ? "Modified" : "Saved");
    }

    void TextEditorView::OpenFileDialog() {
        std::string outPath;
        std::string filter = "All Code Files{.py,.cpp,.hpp,.h,.c,.txt,.md},"
            "Python{.py},"
            "C++{.cpp,.hpp,.h},"
            "C{.c},"
            "Text{.txt},"
            "Markdown{.md},"
            "All Files{.*}";
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DefaultProject") : ".";
        if (FileDialog::OpenFile("Open Code File", filter, outPath, defaultPath)) {
            LoadFile(outPath);
        }
    }

    void TextEditorView::SaveFileDialog() {
        std::string outPath;
        std::string filter;
        if (!currentFilePath.empty()) {
            std::filesystem::path path(currentFilePath);
            std::string ext = path.extension().string();
            if (!ext.empty()) {
                filter = std::string(ext.substr(1)) + "{" + ext + "},All Files{.*}";
            }
            else {
                filter = "All Files{.*}";
            }
        }
        else {
            filter = "All Files{.*}";
        }
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DefaultProject") : ".";
        std::string defaultName = "newfile.txt";
        if (!currentFilePath.empty()) defaultName = std::filesystem::path(currentFilePath).filename().string();
        if (FileDialog::SaveFile("Save Code File", filter, defaultName, outPath, defaultPath)) {
            SaveFile(outPath);
        }
    }

    void TextEditorView::SaveAsFileDialog() {
        std::string outPath;
        std::string filter = "All Code Files{.py,.cpp,.hpp,.h,.c,.txt,.md},"
            "Python{.py},"
            "C++{.cpp,.hpp,.h},"
            "C{.c},"
            "Text{.txt},"
            "Markdown{.md},"
            "All Files{.*}";
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultPath = fileSys ? fileSys->GetPath("DefaultProject") : ".";
        std::string defaultName = "newfile.txt";
        if (!currentFilePath.empty()) defaultName = std::filesystem::path(currentFilePath).filename().string();
        if (FileDialog::SaveFile("Save Code File As", filter, defaultName, outPath, defaultPath)) {
            SaveFile(outPath);
        }
    }

} // namespace GUI