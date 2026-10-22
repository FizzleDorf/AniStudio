#pragma once
#include "BaseSettingsTab.hpp"
#include "FontSettingsComponent.hpp"
#include "IconFonts.hpp"

struct ImGuiContext;

namespace ECS {
    class SettingsSystem;

    class FontSettingsTab : public BaseSettingsTab {
    public:
        FontSettingsTab(SettingsSystem& sys, FontSettingsComponent& comp);
        ~FontSettingsTab() = default;

        std::string GetTabName() const override { return "Fonts"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }
        void SetImGuiContext(void* ctx) override { m_imguiContext = ctx; }

        static void ScanFontsDirectories(FontSettingsComponent& comp);
        static void ApplyFontToImGui(FontSettingsComponent& comp, ImGuiContext* ctx);

    private:
        SettingsSystem& m_settingsSystem;
        FontSettingsComponent& m_comp;
        void* m_imguiContext = nullptr;
        std::string m_filter;

        void RenderFontFamily();
        void RenderIconFontFamily();
        void RenderScaleSchema();
    };

}