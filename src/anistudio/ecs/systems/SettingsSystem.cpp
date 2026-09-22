#include "SettingsSystem.hpp"
#include "EntityManager.hpp"
#include "FilePathSystem.hpp"
#include "GeneralSettingsComponent.hpp"
#include "ImGuiStyleSettingsComponent.hpp"
#include "ImGuiRenderSettingsComponent.hpp"
#include "FontSettingsComponent.hpp"
#include "TextEditorSettingsComponent.hpp"
#include "BaseSettingsTab.hpp"
#include "FilePathTab.hpp"
#include "Log.hpp"

namespace ECS {

    SettingsSystem::SettingsSystem(EntityManager& mgr) : BaseSystem(mgr), settingsEntity(0) {
        sysName = "SettingsSystem";
    }

    void SettingsSystem::Start() {
        ANI_LOG_INFO("[SettingsSystem] Starting");

        filePathSystem = mgr.GetSystem<FilePathSystem>().get();
        settingsEntity = mgr.AddNewEntity();

        RegisterAndAddSettingsComponent<GeneralSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiStyleSettingsComponent>();
        RegisterAndAddSettingsComponent<ImGuiRenderSettingsComponent>();
        RegisterAndAddSettingsComponent<FontSettingsComponent>();
        RegisterAndAddSettingsComponent<TextEditorSettingsComponent>();

        if (filePathSystem) {
            auto& styleComp = mgr.GetComponent<ImGuiStyleSettingsComponent>(settingsEntity);
            styleComp.SetFilePathSystem(filePathSystem);
            auto fileTab = std::make_unique<FilePathTab>(*filePathSystem);
            RegisterTab(std::move(fileTab));
        }
        else {
            ANI_LOG_WARN("[SettingsSystem] FilePathSystem not available; FilePath settings tab not registered");
        }

        ANI_LOG_INFO("[SettingsSystem] Registered %zu settings components, %zu tabs",
            settingsComponentTypes.size(), m_tabs.size());

        LoadAllSettings();
        for (auto& tab : m_tabs) tab->CreateBackup();
    }

    void SettingsSystem::Destroy() {
        ANI_LOG_INFO("[SettingsSystem] Destroying");

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
        ANI_LOG_DEBUG("[SettingsSystem] Registered tab (total=%zu)", m_tabs.size());
    }

    void SettingsSystem::SetImGuiContext(ImGuiContext* context) {
        imguiContext = context;
        for (auto& tab : m_tabs) {
            tab->SetImGuiContext(context);
        }
    }

    bool SettingsSystem::SaveAllSettings() {
        bool success = true;
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            if (!m_tabs[i]->SaveSettings()) {
                ANI_LOG_WARN("[SettingsSystem] SaveSettings failed for tab %zu", i);
                success = false;
            }
        }
        if (success) {
            ANI_LOG_INFO("[SettingsSystem] Saved %zu tabs", m_tabs.size());
        }
        else {
            ANI_LOG_WARN("[SettingsSystem] SaveAllSettings completed with failures");
        }
        return success;
    }

    bool SettingsSystem::LoadAllSettings() {
        bool success = true;
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            if (!m_tabs[i]->LoadSettings()) {
                ANI_LOG_WARN("[SettingsSystem] LoadSettings failed for tab %zu", i);
                success = false;
            }
        }
        if (success) {
            ANI_LOG_INFO("[SettingsSystem] Loaded %zu tabs", m_tabs.size());
        }
        else {
            ANI_LOG_WARN("[SettingsSystem] LoadAllSettings completed with failures");
        }
        return success;
    }

    void SettingsSystem::ResetAllToDefaults() {
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            try {
                m_tabs[i]->ResetToDefaults();
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[SettingsSystem] Exception resetting tab %zu: %s", i, e.what());
            }
        }
    }

    void SettingsSystem::RestoreAllFromBackups() {
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            try {
                m_tabs[i]->RestoreFromBackup();
            }
            catch (const std::exception& e) {
                ANI_LOG_ERROR("[SettingsSystem] Exception restoring tab %zu: %s", i, e.what());
            }
        }
    }

    bool SettingsSystem::HasAnyUnsavedChanges() const {
        for (const auto& tab : m_tabs) {
            if (tab->HasUnsavedChanges()) return true;
        }
        return false;
    }

}