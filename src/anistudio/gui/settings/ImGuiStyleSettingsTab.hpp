#pragma once
#include "BaseSettingsTab.hpp"
#include "ImGuiStyleSettingsComponent.hpp"
#include <vector>
#include <string>

namespace ECS {
    class SettingsSystem;

    class ImGuiStyleSettingsTab : public BaseSettingsTab {
    public:
        ImGuiStyleSettingsTab(SettingsSystem& sys, ImGuiStyleSettingsComponent& comp);
        ~ImGuiStyleSettingsTab() = default;

        std::string GetTabName() const override { return "ImGui Style"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }
        void SetImGuiContext(void* ctx) override { m_imguiContext = ctx; }

    private:
        SettingsSystem& m_settingsSystem;
        ImGuiStyleSettingsComponent& m_comp;
        void* m_imguiContext = nullptr;
        std::string m_filter;

        int m_selectedStyleIndex = 0;
        char m_saveAsFilename[256] = "";
        bool m_showPreview = true;

        std::string GetStylesDirectory() const;
        std::vector<std::string> ScanStyleNames() const;
        std::vector<std::string> ScanStylePaths() const;
        void ApplyBuiltInStyle(int index);
        void ApplyStyleFromFile(const std::string& path);
        void SaveStyleAs(const ImGuiStyle& style, const std::string& filename);
        void SaveCurrentStyle(const std::string& styleName);
        void DeleteStyleFile(const std::string& styleName);

        void RenderStylePresets();
        void RenderMetricSchema();
        void RenderColors();
        void RenderPreviewWindow();
    };

}