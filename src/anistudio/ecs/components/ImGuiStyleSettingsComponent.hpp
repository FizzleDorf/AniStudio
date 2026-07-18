#pragma once
#include "BaseSettingsComponent.hpp"
#include <imgui.h>

namespace ECS {

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

    private:
        ImGuiStyle currentStyle;
        ImGuiStyle backupStyle;
        bool hasChanges = false;
        bool isInitialized = false;
        ImGuiContext* imguiContext = nullptr;

        void EnsureInitialized();
        bool ShowStyleSelector(const char* label);
        void ShowFontSelector(const char* label);
        void RenderActionButtons();
        void SaveStyleToFile(const ImGuiStyle& style, const std::string& filename);
        bool LoadStyleFromFile(ImGuiStyle& style, const std::string& filename);
        void SetCustomDarkTheme();
        bool FilterPass(const std::string& section, const std::string& filter) const;
    };

} // namespace ECS