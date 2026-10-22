#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace ECS {

    class TextEditorSettingsComponent : public BaseComponent {
    public:
        TextEditorSettingsComponent() = default;

        const char* GetCompName() const override { return "TextEditorSettings"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "Text Editor Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "showLineNumbers", "wordWrap", "showWhitespace", "autoIndent", "lineFolding",
                    "useCustomFont", "editorFontName",
                    "tabSize", "defaultLanguage", "autocompleteFile"
                }},
                {"properties", {
                    {"showLineNumbers", {{"type","boolean"},{"title","Show Line Numbers"},{"ui:widget","checkbox"}}},
                    {"wordWrap", {{"type","boolean"},{"title","Word Wrap"},{"ui:widget","checkbox"}}},
                    {"showWhitespace", {{"type","boolean"},{"title","Show Whitespace"},{"ui:widget","checkbox"}}},
                    {"autoIndent", {{"type","boolean"},{"title","Auto Indent"},{"ui:widget","checkbox"}}},
                    {"lineFolding", {{"type","boolean"},{"title","Line Folding"},{"ui:widget","checkbox"}}},
                    {"useCustomFont", {{"type","boolean"},{"title","Use Custom Font"},{"ui:widget","checkbox"}}},
                    {"editorFontName", {{"type","string"},{"title","Editor Font"},{"ui:widget","input_text"}}},
                    {"tabSize", {{"type","integer"},{"title","Tab Size"},{"ui:widget","input_int"},{"ui:options",{{"step",1},{"min",1},{"max",8}}}}},
                    {"defaultLanguage", {
                        {"type","array"},
                        {"title","Default Language"},
                        {"ui:widget","combo"},
                        {"items", {"Plain Text","Python","C++","C","C#","GLSL","HLSL","JSON","Markdown","SQL","Lua","AngelScript"}},
                        {"itemCount", 12}
                    }},
                    {"autocompleteFile", {
                        {"type","string"},
                        {"title","Autocomplete File"},
                        {"ui:widget","file_selector"},
                        {"ui:options", {
                            {"mode","file"},
                            {"filters",".txt,.*"},
                            {"filterName","Keyword Files"},
                            {"dialogDefaultPath","DataPath"},
                            {"buttonText","Browse..."},
                            {"resetButtonText","Clear"}
                        }}
                    }}
                }}
            };
            return j;
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"showLineNumbers", &showLineNumbers},
                {"wordWrap", &wordWrap},
                {"showWhitespace", &showWhitespace},
                {"autoIndent", &autoIndent},
                {"lineFolding", &lineFolding},
                {"useCustomFont", &useCustomFont},
                {"editorFontName", &editorFontName},
                {"tabSize", &tabSize},
                {"defaultLanguage", &defaultLanguage},
                {"autocompleteFile", &autocompleteFile}
            };
        }

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void ResetToDefaults();

        bool showLineNumbers = false;
        bool wordWrap = true;
        bool showWhitespace = false;
        bool autoIndent = true;
        bool lineFolding = false;
        bool useCustomFont = false;
        int tabSize = 4;
        std::string defaultLanguage = "Plain Text";
        std::string autocompleteFile = "python_keywords.txt";
        std::string editorFontName;
    };

}