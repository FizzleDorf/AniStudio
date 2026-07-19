#pragma once
#include "BaseSystem.hpp"
#include <vector>
#include <imgui.h>

namespace ECS {

    class BaseSettingsComponent;
    class FilePathSystem;

    class SettingsSystem : public BaseSystem {
    public:
        SettingsSystem(EntityManager& mgr);

        void Start() override;
        void Destroy() override;
        void Update(float deltaT) override;

        std::vector<BaseSettingsComponent*> GetAllSettingsComponents() const;
        BaseSettingsComponent* GetSettingsComponent(ComponentTypeID typeId) const;

        bool SaveAllSettings();
        bool LoadAllSettings();
        void ResetAllToDefaults();
        void RestoreAllFromBackups();
        bool HasAnyUnsavedChanges() const;

        void SetImGuiContext(ImGuiContext* context);

    private:
        template<typename T>
        ComponentTypeID RegisterAndAddSettingsComponent();

        EntityID settingsEntity = 0;
        std::vector<ComponentTypeID> settingsComponentTypes;
        FilePathSystem* filePathSystem = nullptr;
    };

}