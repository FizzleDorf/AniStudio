#pragma once

#include "BaseSystem.hpp"
#include "Types.hpp"
#include <imgui.h>
#include <vector>

namespace ECS {

    class BaseSettingsComponent;
    class FilePathSystem;

    class SettingsSystem : public BaseSystem {
    public:
        explicit SettingsSystem(EntityManager& mgr);
        virtual ~SettingsSystem() = default;

        void Start() override;
        void Update(float deltaT) override {}
        void Destroy() override;

        std::vector<BaseSettingsComponent*> GetAllSettingsComponents() const;

        bool SaveAllSettings();
        bool LoadAllSettings();
        void ResetAllToDefaults();
        void RestoreAllFromBackups();
        bool HasAnyUnsavedChanges() const;

        void SetImGuiContext(ImGuiContext* context);

        FilePathSystem* GetFilePathSystem() const { return filePathSystem; }

    private:
        EntityID settingsEntity = 0;
        std::vector<ComponentTypeID> settingsComponentTypes;
        FilePathSystem* filePathSystem = nullptr;

        template<typename T>
        ComponentTypeID RegisterAndAddSettingsComponent();

        BaseSettingsComponent* GetSettingsComponent(ComponentTypeID typeId) const;
    };

} // namespace ECS