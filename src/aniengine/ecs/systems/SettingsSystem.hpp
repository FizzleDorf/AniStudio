#pragma once
#include "BaseSystem.hpp"
#include "Types.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

namespace ECS {

    struct BaseComponent;
    class FilePathComponent;

    class SettingsSystem : public BaseSystem {
    public:
        SettingsSystem(EntityManager& mgr);
        ~SettingsSystem() override = default;

        void Start() override;
        void Destroy() override;
        void Update(float deltaT) override {}

        EntityID GetSettingsEntity() const { return settingsEntity; }

        template<typename T>
        ComponentTypeID AddSettingsComponent() {
            ComponentTypeID id = mgr.RegisterComponent<T>(T{}.GetCompName());
            mgr.AddComponent<T>(settingsEntity);
            m_settingsComponentIds.push_back(id);
            return id;
        }

        void TrackSettingsComponent(ComponentTypeID id);

        void SetApplyCallback(ComponentTypeID id, std::function<void(BaseComponent&)> cb);

        void ApplySettings(ComponentTypeID id);
        void ApplySettingsByType(const std::string& compName);

        bool SaveAllSettings();
        bool LoadAllSettings();
        void ApplyAll();
        void CreateBackups();
        void RestoreAllFromBackups();
        bool HasAnyUnsavedChanges() const;

        std::string GetPath(const std::string& key) const;
        void SetPath(const std::string& key, const std::string& path);
        bool HasPath(const std::string& key) const;
        std::vector<std::string> GetAllKeys() const;
        FilePathComponent* GetFilePathComponent() const;

        void RegisterPathDefault(const std::string& key, const std::string& defaultPath);

        void SetMissingPathsCallback(std::function<void(const std::vector<std::string>&)> cb);
        void CheckAndPromptMissingPaths();

        void EnsureDirectoriesFromPaths();

    private:
        std::string GetSettingsDirectory() const;
        std::string GetPathsFile() const;
        static std::string FilenameForComponent(const std::string& compName);

        bool LoadPathComponent();
        bool SavePathComponent();

        EntityID settingsEntity = 0;
        std::vector<ComponentTypeID> m_settingsComponentIds;
        std::unordered_map<ComponentTypeID, std::function<void(BaseComponent&)>> m_applyCallbacks;
        std::unordered_map<ComponentTypeID, nlohmann::json> m_backups;
        std::function<void(const std::vector<std::string>&)> m_missingPathsCallback;
    };

}