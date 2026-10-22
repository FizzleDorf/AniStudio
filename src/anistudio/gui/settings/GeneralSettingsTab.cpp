#include "GeneralSettingsTab.hpp"
#include "SettingsSystem.hpp"
#include "FilePathComponent.hpp"
#include "UISchema.hpp"
#include <imgui.h>

namespace ECS {

    GeneralSettingsTab::GeneralSettingsTab(SettingsSystem& sys, GeneralSettingsComponent& comp)
        : m_settingsSystem(sys), m_comp(comp) {
    }

    void GeneralSettingsTab::Render() {
        auto props = m_comp.GetPropertyMap();
        auto* filePathComp = m_settingsSystem.GetFilePathComponent();
        const std::unordered_map<std::string, std::string>* pathMap =
            filePathComp ? &filePathComp->GetCurrentMap() : nullptr;

        UISchema::RenderSchema(
            m_comp.GetSchema(),
            props,
            nullptr,
            m_comp.GetCompName(),
            (int)m_settingsSystem.GetSettingsEntity(),
            pathMap);
    }

}