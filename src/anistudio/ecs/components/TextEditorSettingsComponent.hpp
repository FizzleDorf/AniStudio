#pragma once
#include "BaseSettingsComponent.hpp"
#include <string>

namespace ECS {

    class TextEditorSettingsComponent : public BaseSettingsComponent {
    public:
        TextEditorSettingsComponent();

        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override;

        bool showLineNumbers = false;
        bool wordWrap = true;
        bool showWhitespace = false;
        bool autoIndent = true;
        bool lineFolding = false;
        bool useCustomFont = false;
        int tabSize = 4;
        std::string defaultLanguage = "Plain Text";
        std::string autocompleteFile = "python_keywords.txt";
        std::string editorFontName = "";

        bool m_hasChanges = false;

    private:
        bool backupShowLineNumbers = false;
        bool backupWordWrap = true;
        bool backupShowWhitespace = false;
        bool backupAutoIndent = true;
        bool backupLineFolding = false;
        bool backupUseCustomFont = false;
        int backupTabSize = 4;
        std::string backupDefaultLanguage;
        std::string backupAutocompleteFile;
        std::string backupEditorFontName;
    };

}