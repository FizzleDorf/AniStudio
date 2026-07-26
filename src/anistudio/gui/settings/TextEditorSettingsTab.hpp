#pragma once
#include "BaseSettingsTab.hpp"
#include "TextEditorSettingsComponent.hpp"
#include "FontSettingsComponent.hpp"

namespace ECS {

    class TextEditorSettingsTab : public BaseSettingsTab {
    public:
        explicit TextEditorSettingsTab(TextEditorSettingsComponent& comp, FontSettingsComponent& fontComp);
        ~TextEditorSettingsTab() = default;

        std::string GetTabName() const override { return "Text Editor"; }
        std::string GetTabCategory() const override { return "Interface"; }
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
        TextEditorSettingsComponent& m_comp;
        FontSettingsComponent& m_fontComp;
        std::string m_filter;
        bool FilterPass(const std::string& section) const;
        void RenderGeneralSettings();
        void RenderFontSettings();
        void RenderActionButtons();
    };

}