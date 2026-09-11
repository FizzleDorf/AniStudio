#pragma once
#include "BaseSettingsComponent.hpp"
#include <imgui.h>
#include <vector>
#include <string>

namespace ECS {
    class FilePathSystem;

    class ImGuiStyleSettingsComponent : public BaseSettingsComponent {
    public:
        ImGuiStyleSettingsComponent();

        const char* GetCompName() const override { return "ImGuiStyleSettingsComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        ImGuiStyleSettingsComponent(const ImGuiStyleSettingsComponent& other);
        ImGuiStyleSettingsComponent& operator=(const ImGuiStyleSettingsComponent& other);

        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }
        void SetImGuiContext(ImGuiContext* context) { imguiContext = context; }

        void SetFilePathSystem(FilePathSystem* system) { m_filePathSystem = system; }

        ImGuiStyle currentStyle;
        ImGuiStyle backupStyle;
        bool hasChanges = false;
        ImGuiContext* imguiContext = nullptr;

        struct StyleFileEntry {
            std::string name;
            std::string path;
        };
        std::vector<StyleFileEntry> availableStyles;
        std::vector<std::string> displayNames;
        int selectedStyleIndex = 0;
        char saveAsFilename[256] = "my_style.json";

        void EnsureInitialized();
        void ScanStylesDirectory();
        void RebuildDisplayList();
        void ApplyBuiltInStyle(int index);
        void ApplyFileStyle(const std::string& path);
        void SaveStyleToFile(const ImGuiStyle& style, const std::string& filename);
        bool LoadStyleFromFile(ImGuiStyle& style, const std::string& filename);
        void SetCustomDarkTheme();
        std::string GetStylesDirectory() const;

    private:
        bool isInitialized = false;
        FilePathSystem* m_filePathSystem = nullptr;
        std::string currentStyleFile;
    };

}