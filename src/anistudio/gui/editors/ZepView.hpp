// ZepView.hpp
#pragma once

#include "GUI.h"
#include "EntityManager.hpp"
#include "ZepUtils.hpp"
#include "PythonComponent.hpp"
#include "PythonSystem.hpp"
#include "FileDialogUtil.hpp"
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>

namespace GUI {
    class ZepView : public BaseView {
    public:
        static constexpr const char* GetMetadataJSON() {
            return R"({
            "displayName": "Text Editor",
            "category": "Tools",
            "description": "Edit text and running python scripts."
        })";
        }

        ZepView(ECS::EntityManager& entityMgr);

        void Init() override;
        void Update(const float deltaT) override;
        void Render() override;

        std::string GetText() const;
        void SetText(const std::string& text);
        bool LoadFile(const std::string& filePath);
        bool SaveFile(const std::string& filePath);

    private:
        std::unique_ptr<Utils::ZepTextEditor> textEditor;
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
        void RenderPythonMenu();
        void RenderVirtualEnvDialog();
        void LoadDefaultPythonScript();
        void CreateDefaultPythonScript(const std::string& filePath);
        void SetDefaultPythonContent();
        std::string GetDefaultPythonContent() const;
        void ExecuteCurrentScript();
        void ClearPythonOutput();
        void OpenFileDialog();
        void SaveAsFileDialog();
        void SaveFileDialog();
    };
}