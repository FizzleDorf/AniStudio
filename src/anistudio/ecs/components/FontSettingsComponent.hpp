#pragma once
#include "BaseSettingsComponent.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace ECS {

    class FontSettingsComponent : public BaseSettingsComponent {
    public:
        using FontRebuildCallback = void(*)();

        FontSettingsComponent() = default;
        ~FontSettingsComponent() = default;

        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }
        void SetImGuiContext(ImGuiContext* context) { imguiContext = context; }

        void ApplyFont();
        void RefreshFontList();
        void CheckAndRebuildFonts();

        static void SetFontRebuildCallback(FontRebuildCallback callback);

        struct FontEntry {
            std::string name;
            std::string path;
        };
        std::vector<FontEntry> availableFonts;
        std::string selectedFontName;
        float m_globalFontScale = 1.0f;
        ImGuiContext* imguiContext = nullptr;
        bool hasChanges = false;

        void EnsureInitialized();
        void ScanFontsDirectory();

    private:
        std::string backupSelectedFontName;
        float backupGlobalFontScale = 1.0f;
        bool isInitialized = false;
        bool fontsScanned = false;
        bool fontsNeedRebuild = false;
        std::string pendingFontPath;

        static FontRebuildCallback s_fontRebuildCallback;

        void RebuildFonts();
    };

}