#pragma once
#include "BaseSettingsTab.hpp"
#include "ImGuiStyleSettingsComponent.hpp"

namespace ECS {

    class ImGuiStyleSettingsTab : public BaseSettingsTab {
    public:
        explicit ImGuiStyleSettingsTab(ImGuiStyleSettingsComponent& comp);
        ~ImGuiStyleSettingsTab() = default;

        std::string GetTabName() const override { return "ImGui Style"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

        bool HasUnsavedChanges() const override { return m_comp.HasUnsavedChanges(); }
        void CreateBackup() override { m_comp.CreateBackup(); }
        void RestoreFromBackup() override { m_comp.RestoreFromBackup(); }
        void ResetToDefaults() override { m_comp.ResetToDefaults(); }
        bool SaveSettings() override { return m_comp.SaveSettings(); }
        bool LoadSettings() override { return m_comp.LoadSettings(); }
        void SetImGuiContext(ImGuiContext* ctx) override { m_comp.SetImGuiContext(ctx); }

    private:
        ImGuiStyleSettingsComponent& m_comp;
        std::string m_filter;

        bool FilterPass(const std::string& section) const;
        void RenderStylePresets();
        void RenderSizeSettings();
        void RenderBorderSettings();
        void RenderRoundingSettings();
        void RenderColorSettings();
        void RenderActionButtons();
    };

}