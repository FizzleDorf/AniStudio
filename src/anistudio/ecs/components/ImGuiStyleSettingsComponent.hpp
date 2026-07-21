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

        std::string GetTabName() const override { return "ImGui Style"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void RenderUI() override;
        void RenderFilteredUI(const std::string& filter) override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }
        void SetImGuiContext(ImGuiContext* context) override { imguiContext = context; }

        void SetFilePathSystem(FilePathSystem* system) { m_filePathSystem = system; }

    private:
        struct StyleFileEntry {
            std::string name;
            std::string path;
        };

        ImGuiStyle currentStyle;
        ImGuiStyle backupStyle;
        bool hasChanges = false;
        bool isInitialized = false;
        ImGuiContext* imguiContext = nullptr;

        FilePathSystem* m_filePathSystem = nullptr;

        std::vector<StyleFileEntry> availableStyles;
        std::vector<std::string> displayNames; // built-in + file names
        int selectedStyleIndex = 0;
        std::string currentStyleFile;
        char saveAsFilename[256] = "my_style.json";

        void EnsureInitialized();
        void ScanStylesDirectory();
        void RebuildDisplayList();
        void ApplyBuiltInStyle(int index);
        void ApplyFileStyle(const std::string& path);
        void SaveStyleToFile(const ImGuiStyle& style, const std::string& filename);
        bool LoadStyleFromFile(ImGuiStyle& style, const std::string& filename);
        void SetCustomDarkTheme();

        void RenderActionButtons();
        bool FilterPass(const std::string& section, const std::string& filter) const;
        std::string GetStylesDirectory() const;
    };
}