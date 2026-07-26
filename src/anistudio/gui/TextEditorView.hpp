#pragma once

#include "GUI.h"
#include "EntityManager.hpp"
#include "PythonComponent.hpp"
#include "PythonSystem.hpp"
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
            "description": "Edit text and running python scripts."
        })";
        }

        TextEditorView(ECS::EntityManager& mgr, ViewManager& vm)
            : BaseView(mgr, vm), pythonEntity(0), showVirtualEnvSettings(false) {
            viewName = "TextEditor";
            textEditor = std::make_unique<TextEditor>();
            venvPathBuffer[0] = '\0';
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
        ECS::EntityID pythonEntity;
        std::string lastDisplayedOutput;
        std::string lastDisplayedError;
        std::string currentFilePath;
        bool showVirtualEnvSettings;

        char venvPathBuffer[512];

        bool IsPythonFile() const;
        void InitializePythonEntity();
        void RenderFileMenu();
        void RenderEditMenu();
        void RenderViewMenu();
        void RenderLanguageMenu();
        void RenderPythonMenu();
        void RenderVirtualEnvDialog();
        void RenderStatusBar();
        void LoadDefaultPythonScript();
        void CreateDefaultPythonScript(const std::string& filePath);
        void SetDefaultPythonContent();
        std::string GetDefaultPythonContent() const;
        void ExecuteCurrentScript();
        void ClearPythonOutput();
        void OpenFileDialog();
        void SaveAsFileDialog();
        void SaveFileDialog();
        size_t ExtractErrorLine(const std::string& error) const;
    };

} // namespace GUI