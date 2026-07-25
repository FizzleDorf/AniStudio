#pragma once
#include "BaseSystem.hpp"
#include <vector>
#include <memory>
#include <imgui.h>

namespace ECS {

    class BaseSettingsTab;
    class BaseSettingsComponent;
    class FilePathSystem;

    class SettingsSystem : public BaseSystem {
    public:
        SettingsSystem(EntityManager& mgr);

        void Start() override;
        void Destroy() override;
        void Update(float deltaT) override;

        EntityID GetSettingsEntity() const { return settingsEntity; }

        void RegisterTab(std::unique_ptr<BaseSettingsTab> tab);
        const std::vector<std::unique_ptr<BaseSettingsTab>>& GetTabs() const { return m_tabs; }

        void SetImGuiContext(ImGuiContext* context);

        bool SaveAllSettings();
        bool LoadAllSettings();
        void ResetAllToDefaults();
        void RestoreAllFromBackups();
        bool HasAnyUnsavedChanges() const;

    private:
        template<typename T>
        ComponentTypeID RegisterAndAddSettingsComponent();

        EntityID settingsEntity;
        std::vector<ComponentTypeID> settingsComponentTypes;
        std::vector<std::unique_ptr<BaseSettingsTab>> m_tabs;
        FilePathSystem* filePathSystem = nullptr;
        ImGuiContext* imguiContext = nullptr;
    };

}