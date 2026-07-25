#pragma once
#include "BaseSettingsTab.hpp"
#include "FontSettingsComponent.hpp"

namespace ECS {

    class FontSettingsTab : public BaseSettingsTab {
    public:
        explicit FontSettingsTab(FontSettingsComponent& comp);
        ~FontSettingsTab() = default;

        std::string GetTabName() const override { return "Fonts"; }
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
        FontSettingsComponent& m_comp;
        std::string m_filter;

        bool FilterPass(const std::string& section) const;
        void RenderFontFamily();
        void RenderScale();
        void RenderActionButtons();
    };

}