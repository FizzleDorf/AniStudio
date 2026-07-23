#include "SettingsSystem.hpp"
#include "EntityManager.hpp"
#include "GeneralSettingsComponent.hpp"
#include "ImGuiStyleSettingsComponent.hpp"
#include "ImGuiRenderSettingsComponent.hpp"
#include "FontSettingsComponent.hpp"
#include "FilePathSystem.hpp"
#include <iostream>

namespace ECS {

    SettingsSystem::SettingsSystem(EntityManager& mgr) : BaseSystem(mgr), settingsEntity(mgr.AddNewEntity()) {
        sysName = "SettingsSystem";
    }

    void SettingsSystem::Start() {
        filePathSystem = mgr.GetSystem<FilePathSystem>().get();
        if (!filePathSystem) {
            std::cerr << "[SettingsSystem] WARNING: FilePathSystem not found." << std::endl;
        }

        settingsEntity = mgr.AddNewEntity();

        RegisterAndAddSettingsComponent<GeneralSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiStyleSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiRenderSettingsComponent>();
        RegisterAndAddSettingsComponent<FontSettingsComponent>();

        LoadAllSettings();
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp) comp->CreateBackup();
        }
    }

    void SettingsSystem::Destroy() {
        if (mgr.IsEntityValid(settingsEntity)) {
            mgr.DestroyEntity(settingsEntity);
            settingsEntity = 0;
        }
        settingsComponentTypes.clear();
        filePathSystem = nullptr;
    }

    void SettingsSystem::Update(float deltaT) {
        if (mgr.IsEntityValid(settingsEntity) && mgr.HasComponent<FontSettingsComponent>(settingsEntity)) {
            auto& fontComp = mgr.GetComponent<FontSettingsComponent>(settingsEntity);
            fontComp.CheckAndRebuildFonts();
        }
    }

    std::vector<BaseSettingsComponent*> SettingsSystem::GetAllSettingsComponents() const {
        std::vector<BaseSettingsComponent*> comps;
        for (ComponentTypeID typeId : settingsComponentTypes) {
            auto* comp = GetSettingsComponent(typeId);
            if (comp) comps.push_back(comp);
        }
        return comps;
    }

    BaseSettingsComponent* SettingsSystem::GetSettingsComponent(ComponentTypeID typeId) const {
        if (!mgr.IsEntityValid(settingsEntity)) return nullptr;
        auto* base = mgr.GetComponentById(settingsEntity, typeId);
        return dynamic_cast<BaseSettingsComponent*>(base);
    }

    template<typename T>
    ComponentTypeID SettingsSystem::RegisterAndAddSettingsComponent() {
        ComponentTypeID typeId = mgr.RegisterComponent<T>(typeid(T).name());
        mgr.AddComponent<T>(settingsEntity);
        settingsComponentTypes.push_back(typeId);
        return typeId;
    }

    bool SettingsSystem::SaveAllSettings() {
        bool success = true;
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp && !comp->SaveSettings()) {
                success = false;
            }
        }
        return success;
    }

    bool SettingsSystem::LoadAllSettings() {
        bool success = true;
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp && !comp->LoadSettings()) {
                success = false;
            }
        }
        return success;
    }

    void SettingsSystem::ResetAllToDefaults() {
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp) comp->ResetToDefaults();
        }
    }

    void SettingsSystem::RestoreAllFromBackups() {
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp) comp->RestoreFromBackup();
        }
    }

    bool SettingsSystem::HasAnyUnsavedChanges() const {
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp && comp->HasUnsavedChanges()) return true;
        }
        return false;
    }

    void SettingsSystem::SetImGuiContext(ImGuiContext* context) {
        for (auto* comp : GetAllSettingsComponents()) {
            if (comp) comp->SetImGuiContext(context);
        }
    }

}