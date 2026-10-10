#include "TextEditorSettingsComponent.hpp"

namespace ECS {

    nlohmann::json TextEditorSettingsComponent::Serialize() const {
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
        return j;
    }

    void TextEditorSettingsComponent::Deserialize(const nlohmann::json& j) {
        showLineNumbers = j.value("showLineNumbers", false);
        wordWrap = j.value("wordWrap", true);
        showWhitespace = j.value("showWhitespace", false);
        autoIndent = j.value("autoIndent", true);
        lineFolding = j.value("lineFolding", false);
        useCustomFont = j.value("useCustomFont", false);
        tabSize = j.value("tabSize", 4);
        defaultLanguage = j.value("defaultLanguage", std::string("Plain Text"));
        autocompleteFile = j.value("autocompleteFile", std::string("python_keywords.txt"));
        editorFontName = j.value("editorFontName", std::string(""));
    }

    void TextEditorSettingsComponent::ResetToDefaults() {
        *this = TextEditorSettingsComponent{};
    }

}