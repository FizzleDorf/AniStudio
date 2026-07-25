#include "SettingsSystem.hpp"
#include "EntityManager.hpp"
#include "FilePathSystem.hpp"
#include "GeneralSettingsComponent.hpp"
#include "ImGuiStyleSettingsComponent.hpp"
#include "ImGuiRenderSettingsComponent.hpp"
#include "FontSettingsComponent.hpp"
#include "BaseSettingsTab.hpp"
#include "FilePathTab.hpp"
#include <iostream>

namespace ECS {

    SettingsSystem::SettingsSystem(EntityManager& mgr) : BaseSystem(mgr), settingsEntity(0) {
        sysName = "SettingsSystem";
    }

    void SettingsSystem::Start() {
        filePathSystem = mgr.GetSystem<FilePathSystem>().get();
        settingsEntity = mgr.AddNewEntity();

        RegisterAndAddSettingsComponent<GeneralSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiStyleSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiRenderSettingsComponent>();
        RegisterAndAddSettingsComponent<FontSettingsComponent>();

        if (filePathSystem) {
            auto fileTab = std::make_unique<FilePathTab>(*filePathSystem);
            RegisterTab(std::move(fileTab));
        }

        LoadAllSettings();
        for (auto& tab : m_tabs) tab->CreateBackup();
    }

    void SettingsSystem::Destroy() {
        if (mgr.IsEntityValid(settingsEntity)) {
            mgr.DestroyEntity(settingsEntity);
            settingsEntity = 0;
        }
        settingsComponentTypes.clear();
        m_tabs.clear();
        filePathSystem = nullptr;
    }

    void SettingsSystem::Update(float deltaT) {
        if (mgr.IsEntityValid(settingsEntity) && mgr.HasComponent<FontSettingsComponent>(settingsEntity)) {
            auto& fontComp = mgr.GetComponent<FontSettingsComponent>(settingsEntity);
            fontComp.CheckAndRebuildFonts();
        }
    }

    template<typename T>
    ComponentTypeID SettingsSystem::RegisterAndAddSettingsComponent() {
        ComponentTypeID typeId = mgr.RegisterComponent<T>(typeid(T).name());
        mgr.AddComponent<T>(settingsEntity);
        settingsComponentTypes.push_back(typeId);
        return typeId;
    }

    void SettingsSystem::RegisterTab(std::unique_ptr<BaseSettingsTab> tab) {
        if (imguiContext) tab->SetImGuiContext(imguiContext);
        m_tabs.push_back(std::move(tab));
    }

    void SettingsSystem::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
        for (auto& tab : m_tabs) {
            tab->SetImGuiContext(context);
        }
    }

    bool SettingsSystem::SaveAllSettings() {
        bool success = true;
        for (auto& tab : m_tabs) {
            if (!tab->SaveSettings()) success = false;
        }
        return success;
    }

    bool SettingsSystem::LoadAllSettings() {
        bool success = true;
        for (auto& tab : m_tabs) {
            if (!tab->LoadSettings()) success = false;
        }
        return success;
    }

    void SettingsSystem::ResetAllToDefaults() {
        for (auto& tab : m_tabs) {
            tab->ResetToDefaults();
        }
    }

    void SettingsSystem::RestoreAllFromBackups() {
        for (auto& tab : m_tabs) {
            tab->RestoreFromBackup();
        }
    }

    bool SettingsSystem::HasAnyUnsavedChanges() const {
        for (const auto& tab : m_tabs) {
            if (tab->HasUnsavedChanges()) return true;
        }
        return false;
    }

}