#pragma once

#include "BaseSettingsComponent.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace ECS {

    class FontSettingsComponent : public BaseSettingsComponent {
    public:
        using FontRebuildCallback = void(*)();

        FontSettingsComponent();
        ~FontSettingsComponent() { if (s_instance == this) s_instance = nullptr; }

        std::string GetTabName() const override { return "Fonts"; }
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

        void ApplyFont();
        void RefreshFontList();
        void CheckAndRebuildFonts();

        static void SetFontRebuildCallback(FontRebuildCallback callback);
        static FontSettingsComponent* GetInstance() { return s_instance; }

    private:
        struct FontEntry {
            std::string name;
            std::string path;
        };

        std::vector<FontEntry> availableFonts;
        std::string selectedFontName;
        float m_globalFontScale = 1.0f;

        std::string backupSelectedFontName;
        float backupGlobalFontScale = 1.0f;

        bool hasChanges = false;
        bool isInitialized = false;
        bool fontsScanned = false;
        ImGuiContext* imguiContext = nullptr;

        bool fontsNeedRebuild = false;
        std::string pendingFontPath;

        static FontRebuildCallback s_fontRebuildCallback;
        static FontSettingsComponent* s_instance;

        void EnsureInitialized();
        void ScanFontsDirectory();
        void RebuildFonts();
        bool FilterPass(const std::string& section, const std::string& filter) const;
        void RenderActionButtons();
    };

} // namespace ECS