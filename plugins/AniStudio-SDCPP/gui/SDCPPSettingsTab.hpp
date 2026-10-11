// SDCPPSettingsTab.hpp
#pragma once
#include "BaseSettingsTab.hpp"
#include "SDCPPSettingsComponent.hpp"

namespace ECS {

    class SDCPPSettingsTab : public BaseSettingsTab {
    public:
        explicit SDCPPSettingsTab(SDCPPSettingsComponent& comp);
        ~SDCPPSettingsTab() = default;

        std::string GetTabName() const override { return "SDCPP"; }
        std::string GetTabCategory() const override { return "SDCPP"; }
        void Render() override;
        void SetFilter(const std::string& filter) override { m_filter = filter; }

    private:
        SDCPPSettingsComponent& m_comp;
        std::string m_filter;

        bool FilterPass(const std::string& section) const;
    };

}