#pragma once
#include "BaseSettingsTab.hpp"
#include "GeneralSettingsComponent.hpp"

namespace ECS {
    class SettingsSystem;

    class GeneralSettingsTab : public BaseSettingsTab {
    public:
        GeneralSettingsTab(SettingsSystem& sys, GeneralSettingsComponent& comp);
        ~GeneralSettingsTab() = default;

        std::string GetTabName() const override { return "General"; }
        std::string GetTabCategory() const override { return "Application"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

    private:
        SettingsSystem& m_settingsSystem;
        GeneralSettingsComponent& m_comp;
        std::string m_filter;
    };

}