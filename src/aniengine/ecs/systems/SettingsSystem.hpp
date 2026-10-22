#pragma once
#include "BaseSystem.hpp"
#include "Types.hpp"
#include "EntityManager.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <fstream>
#include <filesystem>

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
            ComponentTypeID id = mgr.template RegisterComponent<T>(T{}.GetCompName());
            mgr.template AddComponent<T>(settingsEntity);
            m_settingsComponentIds.push_back(id);
            return id;
        }

        void TrackSettingsComponent(ComponentTypeID id);

        void SetApplyCallback(ComponentTypeID id, std::function<void(BaseComponent&)> cb);

        void ApplySettings(ComponentTypeID id);
        void ApplySettingsByType(const std::string& compName);

        bool LoadAllSettings();
        bool SaveAllSettings();
        void ApplyAll();

        void TakeSnapshot();
        void RestoreFromSnapshot();
        bool HasUnsavedChanges() const;

        std::string GetPath(const std::string& key) const;
        void SetPath(const std::string& key, const std::string& path);
        bool HasPath(const std::string& key) const;
        std::vector<std::string> GetAllKeys() const;
        FilePathComponent* GetFilePathComponent() const;

        void RegisterPathDefault(const std::string& key, const std::string& defaultPath);

        void SetMissingPathsCallback(std::function<void(const std::vector<std::string>&)> cb);
        void CheckAndPromptMissingPaths();

        void EnsureDirectoriesFromPaths();

        template<typename T>
        bool LoadComponentSettings(T& comp) {
            std::string path = GetComponentFilePath(comp.GetCompName());
            if (!std::filesystem::exists(path)) return false;
            try {
                std::ifstream f(path);
                if (!f.is_open()) return false;
                nlohmann::json j;
                f >> j;
                comp.Deserialize(j);
                return true;
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[SettingsSystem] LoadComponentSettings failed for %s: %s",
                    comp.GetCompName(), e.what());
                return false;
            }
        }

        template<typename T>
        bool SaveComponentSettings(const T& comp) {
            std::string path = GetComponentFilePath(comp.GetCompName());
            try {
                std::filesystem::path p(path);
                if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
                std::ofstream f(path);
                if (!f.is_open()) return false;
                nlohmann::json j = comp.Serialize();
                f << j.dump(4);
                return true;
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[SettingsSystem] SaveComponentSettings failed for %s: %s",
                    comp.GetCompName(), e.what());
                return false;
            }
        }

    private:
        std::string GetSettingsDirectory() const;
        std::string GetPathsFile() const;
        std::string GetComponentFilePath(const std::string& compName) const;

        bool LoadPathComponent();
        bool SavePathComponent();

        EntityID settingsEntity = 0;
        std::vector<ComponentTypeID> m_settingsComponentIds;
        std::unordered_map<ComponentTypeID, std::function<void(BaseComponent&)>> m_applyCallbacks;
        std::unordered_map<ComponentTypeID, nlohmann::json> m_snapshot;
        std::function<void(const std::vector<std::string>&)> m_missingPathsCallback;
    };

}