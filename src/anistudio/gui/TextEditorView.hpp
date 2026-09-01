#pragma once

#include "GUI.h"
#include "EntityManager.hpp"
#include "FileDialogUtil.hpp"
#include <TextEditor.h>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

namespace GUI {

    class TextEditorView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Text Editor",
            "category": "Tools",
            "description": "Edit text files."
        })";
        }

        TextEditorView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm) {
            viewName = "TextEditor";
            textEditor = std::make_unique<TextEditor>();
        }
        ~TextEditorView() = default;

        void Init() override;
        void Update(const float deltaT) override;
        void Render() override;

        std::string GetText() const;
        void SetText(const std::string& text);
        bool LoadFile(const std::string& filePath);
        bool SaveFile(const std::string& filePath);

        std::string GetWindowTitle() const override;

    private:
        std::unique_ptr<TextEditor> textEditor;
        std::string lastDisplayedOutput;
        std::string lastDisplayedError;
        std::string currentFilePath;

        void RenderFileMenu();
        void RenderEditMenu();
        void RenderViewMenu();
        void RenderLanguageMenu();
        void RenderStatusBar();
        void OpenFileDialog();
        void SaveAsFileDialog();
        void SaveFileDialog();
    };

} // namespace GUI