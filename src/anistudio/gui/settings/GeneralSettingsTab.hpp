#pragma once
#include "BaseSettingsTab.hpp"
#include "GeneralSettingsComponent.hpp"

namespace ECS {

    class GeneralSettingsTab : public BaseSettingsTab {
    public:
        explicit GeneralSettingsTab(GeneralSettingsComponent& comp);
        ~GeneralSettingsTab() = default;

        std::string GetTabName() const override { return "General"; }
        std::string GetTabCategory() const override { return "Application"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

        bool HasUnsavedChanges() const override { return m_comp.HasUnsavedChanges(); }
        void CreateBackup() override { m_comp.CreateBackup(); }
        void RestoreFromBackup() override { m_comp.RestoreFromBackup(); }
        void ResetToDefaults() override { m_comp.ResetToDefaults(); }
        bool SaveSettings() override { return m_comp.SaveSettings(); }
        bool LoadSettings() override { return m_comp.LoadSettings(); }
        void SetImGuiContext(ImGuiContext* ctx) override {}

    private:
        GeneralSettingsComponent& m_comp;
        std::string m_filter;

        bool FilterPass(const std::string& section) const;
        void RenderStartupSettings();
        void RenderAutoSaveSettings();
        void RenderConfirmationSettings();
        void RenderPerformanceSettings();
        void RenderLoggingSettings();
        void RenderPluginSettings();
        void RenderActionButtons();
    };

}