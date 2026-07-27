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

        LoadDefaultPythonScript();
    }

    void TextEditorView::Update(const float deltaT) {
        if (IsPythonFile() && pythonEntity != 0 && m_entityManager.HasComponent<ECS::PythonComponent>(pythonEntity)) {
            auto& pythonComp = m_entityManager.GetComponent<ECS::PythonComponent>(pythonEntity);

            if (!pythonComp.output.empty() && pythonComp.output != lastDisplayedOutput) {
                std::cout << "Python Output: " << pythonComp.output << std::endl;
                lastDisplayedOutput = pythonComp.output;
            }

            if (!pythonComp.error.empty() && pythonComp.error != lastDisplayedError) {
                std::cerr << "Python Error: " << pythonComp.error << std::endl;
                lastDisplayedError = pythonComp.error;

                TextEditorUtil::clearMarkers(textEditor.get());
                size_t line = ExtractErrorLine(pythonComp.error);
                if (line < textEditor->GetLineCount()) {
                    TextEditorUtil::addErrorMarker(textEditor.get(), line, pythonComp.error);
                }
            }
        }
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
                if (IsPythonFile()) {
                    RenderPythonMenu();
                }
                ImGui::EndMenuBar();
            }

            if (IsPythonFile() && showVirtualEnvSettings) {
                RenderVirtualEnvDialog();
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

        if (IsPythonFile()) {
            if (pythonEntity == 0) {
                InitializePythonEntity();
            }
            std::string keywordsFile = std::filesystem::path(filePath).parent_path().string() + "/python_keywords.txt";
            TextEditorUtil::setupAutocomplete(textEditor.get(), keywordsFile);
        }

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

    bool TextEditorView::IsPythonFile() const {
        if (currentFilePath.empty()) {
            return false;
        }
        std::filesystem::path path(currentFilePath);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        return extension == ".py";
    }

    void TextEditorView::InitializePythonEntity() {
        if (!m_entityManager.GetSystem<ECS::PythonSystem>()) {
            m_entityManager.RegisterSystem<ECS::PythonSystem>();
            std::cout << "PythonSystem registered" << std::endl;
        }
        pythonEntity = m_entityManager.AddNewEntity();
        m_entityManager.AddComponent<ECS::PythonComponent>(pythonEntity);
        std::cout << "Python entity created for script execution" << std::endl;
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
            ImGui::Separator();
            if (ImGui::MenuItem("Load Default Python Script")) {
                LoadDefaultPythonScript();
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

    void TextEditorView::RenderPythonMenu() {
        if (ImGui::BeginMenu("Python")) {
            if (ImGui::MenuItem("Execute Script", "F5")) {
                ExecuteCurrentScript();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Output")) {
                ClearPythonOutput();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Virtual Environment Settings")) {
                showVirtualEnvSettings = true;
            }
            ImGui::Separator();
            if (pythonEntity != 0 && m_entityManager.HasComponent<ECS::PythonComponent>(pythonEntity)) {
                auto& pythonComp = m_entityManager.GetComponent<ECS::PythonComponent>(pythonEntity);
                if (pythonComp.useVirtualEnv) {
                    std::string venvName = pythonComp.virtualEnvPath.empty() ?
                        "Default" : std::filesystem::path(pythonComp.virtualEnvPath).filename().string();
                    ImGui::MenuItem(("Virtual Env: " + venvName).c_str(), nullptr, false, false);
                }
                else {
                    ImGui::MenuItem("Using: System Python", nullptr, false, false);
                }
            }
            ImGui::EndMenu();
        }
    }

    void TextEditorView::RenderVirtualEnvDialog() {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Virtual Environment Settings", &showVirtualEnvSettings)) {
            if (pythonEntity != 0 && m_entityManager.HasComponent<ECS::PythonComponent>(pythonEntity)) {
                auto& pythonComp = m_entityManager.GetComponent<ECS::PythonComponent>(pythonEntity);
                ImGui::Text("Python Virtual Environment Configuration");
                ImGui::Separator();
                ImGui::Checkbox("Use Virtual Environment", &pythonComp.useVirtualEnv);
                if (pythonComp.useVirtualEnv) {
                    ImGui::Text("Virtual Environment Path:");
                    if (strlen(venvPathBuffer) == 0) {
                        strncpy(venvPathBuffer, pythonComp.virtualEnvPath.c_str(), sizeof(venvPathBuffer) - 1);
                    }
                    if (ImGui::InputText("##VenvPath", venvPathBuffer, sizeof(venvPathBuffer))) {
                        pythonComp.virtualEnvPath = venvPathBuffer;
                        pythonComp.UpdateVirtualEnvPaths();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Browse##Venv")) {
                        std::string folderPath;
                        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                        std::string defaultPath = fileSys ? fileSys->GetPath("DefaultProject") : ".";
                        if (FileDialog::SelectFolder("Choose Virtual Environment Directory", folderPath, defaultPath)) {
                            pythonComp.virtualEnvPath = folderPath;
                            pythonComp.UpdateVirtualEnvPaths();
                            strncpy(venvPathBuffer, folderPath.c_str(), sizeof(venvPathBuffer) - 1);
                        }
                    }
                    if (ImGui::Button("Reset to Default")) {
                        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
                        pythonComp.virtualEnvPath = fileSys ? fileSys->GetPath("VirtualEnv") : "";
                        pythonComp.UpdateVirtualEnvPaths();
                        strncpy(venvPathBuffer, pythonComp.virtualEnvPath.c_str(), sizeof(venvPathBuffer) - 1);
                    }
                    ImGui::Text("Python Executable: %s", pythonComp.pythonExecutable.c_str());
                    ImGui::Text("Site Packages: %s", pythonComp.sitePackagesPath.c_str());
                    if (std::filesystem::exists(pythonComp.pythonExecutable)) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Virtual environment found");
                    }
                    else {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Virtual environment not found");
                        ImGui::Text("Create virtual environment with: python -m venv %s", pythonComp.virtualEnvPath.c_str());
                    }
                }
                else {
                    ImGui::Text("Using system Python interpreter");
                }
                ImGui::Separator();
                if (ImGui::Button("Close")) {
                    showVirtualEnvSettings = false;
                }
            }
        }
        ImGui::End();
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

    void TextEditorView::LoadDefaultPythonScript() {
        auto fileSys = m_entityManager.GetSystem<ECS::FilePathSystem>();
        std::string defaultProjectPath = fileSys ? fileSys->GetPath("DefaultProject") : ".";
        std::string defaultScriptPath = defaultProjectPath + "/scripts/default_script.py";

        try {
            std::string scriptsDir = defaultProjectPath + "/scripts";
            std::filesystem::create_directories(scriptsDir);

            if (std::filesystem::exists(defaultScriptPath)) {
                if (LoadFile(defaultScriptPath)) {
                    std::cout << "Default Python script loaded from: " << defaultScriptPath << std::endl;
                    return;
                }
            }
            else {
                CreateDefaultPythonScript(defaultScriptPath);
                if (LoadFile(defaultScriptPath)) {
                    std::cout << "Created and loaded default Python script: " << defaultScriptPath << std::endl;
                    return;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load default Python script: " << e.what() << std::endl;
        }

        SetDefaultPythonContent();
        currentFilePath = defaultScriptPath;

        if (pythonEntity == 0) {
            InitializePythonEntity();
        }

        std::cout << "Loaded default Python script content" << std::endl;
    }

    void TextEditorView::CreateDefaultPythonScript(const std::string& filePath) {
        try {
            std::ofstream file(filePath);
            if (file.is_open()) {
                file << GetDefaultPythonContent();
                file.close();
                std::cout << "Created default Python script at: " << filePath << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to create default Python script: " << e.what() << std::endl;
        }
    }

    void TextEditorView::SetDefaultPythonContent() {
        SetText(GetDefaultPythonContent());
    }

    std::string TextEditorView::GetDefaultPythonContent() const {
        return R"(# AniStudio Default Python Script
# This script demonstrates Python integration with AniStudio

import math
import random
import sys
from datetime import datetime

def main():
    """Main function demonstrating AniStudio Python integration"""

    print("=" * 50)
    print("Welcome to AniStudio Python Environment!")
    print("=" * 50)

    # System Information
    print(f"\nPython Version: {sys.version}")
    print(f"Executable: {sys.executable}")
    print(f"Current Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    # Math Demonstrations
    print(f"\nMathematical Constants:")
    print(f"   Pi: {math.pi:.6f}")
    print(f"   e (Euler): {math.e:.6f}")

    # Random Number Generation
    print(f"\nRandom Number Generation:")
    random_numbers = [random.randint(1, 100) for _ in range(5)]
    print(f"   Random integers (1-100): {random_numbers}")
    print(f"   Average: {sum(random_numbers) / len(random_numbers):.2f}")

    # String Manipulations
    print(f"\nString Manipulations:")
    message = "AniStudio: Media Creation Made Easy!"
    print(f"   Original: {message}")
    print(f"   Reversed: {message[::-1]}")
    print(f"   Word Count: {len(message.split())} words")

    # Media Creation Workflow Example
    print(f"\nMedia Creation Workflow Example:")
    project_settings = {
        "resolution": "1920x1080",
        "framerate": 30,
        "format": "MP4",
        "quality": "High"
    }

    print("   Project Settings:")
    for key, value in project_settings.items():
        print(f"     {key.capitalize()}: {value}")

    print(f"\nScript execution completed successfully!")
    print("=" * 50)

if __name__ == "__main__":
    main()
    print("\nReady for your AniStudio projects!")
)";
    }

    void TextEditorView::ExecuteCurrentScript() {
        if (!IsPythonFile()) {
            std::cout << "Current file is not a Python script!" << std::endl;
            return;
        }

        std::string scriptText = GetText();
        if (!scriptText.empty() && pythonEntity != 0) {
            auto& pythonComp = m_entityManager.GetComponent<ECS::PythonComponent>(pythonEntity);

            pythonComp.output.clear();
            pythonComp.error.clear();
            lastDisplayedOutput.clear();
            lastDisplayedError.clear();

            pythonComp.script = scriptText;
            pythonComp.isFile = false;
            pythonComp.execute = true;

            std::cout << "Executing Python script through PythonSystem..." << std::endl;
        }
        else {
            std::cout << "No script to execute or Python entity not initialized!" << std::endl;
        }
    }

    void TextEditorView::ClearPythonOutput() {
        if (pythonEntity != 0 && m_entityManager.HasComponent<ECS::PythonComponent>(pythonEntity)) {
            auto& pythonComp = m_entityManager.GetComponent<ECS::PythonComponent>(pythonEntity);
            pythonComp.output.clear();
            pythonComp.error.clear();
            lastDisplayedOutput.clear();
            lastDisplayedError.clear();
            std::cout << "Python output cleared" << std::endl;
        }
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
        if (IsPythonFile()) {
            filter = "Python{.py},All Files{.*}";
        }
        else if (!currentFilePath.empty()) {
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
        if (IsPythonFile()) defaultName = "script.py";
        else if (!currentFilePath.empty()) defaultName = std::filesystem::path(currentFilePath).filename().string();
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
        if (IsPythonFile()) defaultName = "script.py";
        else if (!currentFilePath.empty()) defaultName = std::filesystem::path(currentFilePath).filename().string();
        if (FileDialog::SaveFile("Save Code File As", filter, defaultName, outPath, defaultPath)) {
            SaveFile(outPath);
        }
    }

    size_t TextEditorView::ExtractErrorLine(const std::string& error) const {
        std::regex lineRegex(R"(line\s+(\d+))", std::regex::icase);
        std::smatch match;
        if (std::regex_search(error, match, lineRegex) && match.size() > 1) {
            return static_cast<size_t>(std::stoi(match[1].str())) - 1;
        }
        return static_cast<size_t>(-1);
    }

} // namespace GUI