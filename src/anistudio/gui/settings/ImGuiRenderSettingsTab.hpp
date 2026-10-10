#pragma once
#include "BaseSettingsTab.hpp"
#include "ImGuiRenderSettingsComponent.hpp"

namespace ECS {
    class SettingsSystem;

    class ImGuiRenderSettingsTab : public BaseSettingsTab {
    public:
        ImGuiRenderSettingsTab(SettingsSystem& sys, ImGuiRenderSettingsComponent& comp);
        ~ImGuiRenderSettingsTab() = default;

        std::string GetTabName() const override { return "ImGui Render"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }
        void SetImGuiContext(void* ctx) override { m_imguiContext = ctx; }

        static void ApplyToImGui(ImGuiRenderSettingsComponent& comp, ImGuiContext* ctx);

    private:
        SettingsSystem& m_settingsSystem;
        ImGuiRenderSettingsComponent& m_comp;
        void* m_imguiContext = nullptr;
        std::string m_filter;
    };

}