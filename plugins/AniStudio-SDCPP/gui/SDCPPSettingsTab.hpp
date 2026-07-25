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

        bool HasUnsavedChanges() const override { return m_comp.HasUnsavedChanges(); }
        void CreateBackup() override { m_comp.CreateBackup(); }
        void RestoreFromBackup() override { m_comp.RestoreFromBackup(); }
        void ResetToDefaults() override { m_comp.ResetToDefaults(); }
        bool SaveSettings() override { return m_comp.SaveSettings(); }
        bool LoadSettings() override { return m_comp.LoadSettings(); }
        void SetImGuiContext(ImGuiContext* ctx) override {}

    private:
        SDCPPSettingsComponent& m_comp;
        std::string m_filter;

        bool FilterPass(const std::string& section) const;
        void RenderGeneralSettings();
        void RenderModelSettings();
        void RenderAdvancedSettings();
        void RenderActionButtons();
    };

} // namespace ECS