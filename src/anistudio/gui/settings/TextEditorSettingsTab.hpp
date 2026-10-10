#pragma once
#include "BaseSettingsTab.hpp"
#include "TextEditorSettingsComponent.hpp"
#include "FontSettingsComponent.hpp"

namespace ECS {
    class SettingsSystem;

    class TextEditorSettingsTab : public BaseSettingsTab {
    public:
        TextEditorSettingsTab(SettingsSystem& sys,
            TextEditorSettingsComponent& comp,
            FontSettingsComponent& fontComp);
        ~TextEditorSettingsTab() = default;

        std::string GetTabName() const override { return "Text Editor"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

    private:
        SettingsSystem& m_settingsSystem;
        TextEditorSettingsComponent& m_comp;
        FontSettingsComponent& m_fontComp;
        std::string m_filter;

        void RenderFontOverride();
    };

}