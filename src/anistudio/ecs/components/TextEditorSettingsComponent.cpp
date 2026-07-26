#include "TextEditorSettingsComponent.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace ECS {

    TextEditorSettingsComponent::TextEditorSettingsComponent() {
        compName = "TextEditorSettingsComponent";
        ResetToDefaults();
        CreateBackup();
    }

    bool TextEditorSettingsComponent::SaveSettings() {
        std::string dir = GetSettingsDirectory();
        std::filesystem::create_directories(dir);
        std::string path = dir + "/text_editor.json";
        nlohmann::json j;
        j["showLineNumbers"] = showLineNumbers;
        j["wordWrap"] = wordWrap;
        j["showWhitespace"] = showWhitespace;
        j["autoIndent"] = autoIndent;
        j["lineFolding"] = lineFolding;
        j["useCustomFont"] = useCustomFont;
        j["tabSize"] = tabSize;
        j["defaultLanguage"] = defaultLanguage;
        j["autocompleteFile"] = autocompleteFile;
        j["editorFontName"] = editorFontName;
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        m_hasChanges = false;
        CreateBackup();
        return true;
    }

    bool TextEditorSettingsComponent::LoadSettings() {
        std::string path = GetSettingsDirectory() + "/text_editor.json";
        std::ifstream file(path);
        if (!file.is_open()) {
            ResetToDefaults();
            m_hasChanges = false;
            return true;
        }
        nlohmann::json j;
        file >> j;
        showLineNumbers = j.value("showLineNumbers", false);
        wordWrap = j.value("wordWrap", true);
        showWhitespace = j.value("showWhitespace", false);
        autoIndent = j.value("autoIndent", true);
        lineFolding = j.value("lineFolding", false);
        useCustomFont = j.value("useCustomFont", false);
        tabSize = j.value("tabSize", 4);
        defaultLanguage = j.value("defaultLanguage", "Plain Text");
        autocompleteFile = j.value("autocompleteFile", "python_keywords.txt");
        editorFontName = j.value("editorFontName", "");
        m_hasChanges = false;
        CreateBackup();
        return true;
    }

    void TextEditorSettingsComponent::ResetToDefaults() {
        showLineNumbers = false;
        wordWrap = true;
        showWhitespace = false;
        autoIndent = true;
        lineFolding = false;
        useCustomFont = false;
        tabSize = 4;
        defaultLanguage = "Plain Text";
        autocompleteFile = "python_keywords.txt";
        editorFontName = "";
        m_hasChanges = true;
    }

    void TextEditorSettingsComponent::CreateBackup() {
        backupShowLineNumbers = showLineNumbers;
        backupWordWrap = wordWrap;
        backupShowWhitespace = showWhitespace;
        backupAutoIndent = autoIndent;
        backupLineFolding = lineFolding;
        backupUseCustomFont = useCustomFont;
        backupTabSize = tabSize;
        backupDefaultLanguage = defaultLanguage;
        backupAutocompleteFile = autocompleteFile;
        backupEditorFontName = editorFontName;
    }

    void TextEditorSettingsComponent::RestoreFromBackup() {
        showLineNumbers = backupShowLineNumbers;
        wordWrap = backupWordWrap;
        showWhitespace = backupShowWhitespace;
        autoIndent = backupAutoIndent;
        lineFolding = backupLineFolding;
        useCustomFont = backupUseCustomFont;
        tabSize = backupTabSize;
        defaultLanguage = backupDefaultLanguage;
        autocompleteFile = backupAutocompleteFile;
        editorFontName = backupEditorFontName;
        m_hasChanges = false;
    }

    bool TextEditorSettingsComponent::HasUnsavedChanges() const {
        return m_hasChanges;
    }

}