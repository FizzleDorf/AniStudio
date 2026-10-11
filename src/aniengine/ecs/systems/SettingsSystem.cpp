#include "SettingsSystem.hpp"
#include "EntityManager.hpp"
#include "BaseComponent.hpp"
#include "FilePathComponent.hpp"
#include "Log.hpp"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace ECS {

    SettingsSystem::SettingsSystem(EntityManager& mgr)
        : BaseSystem(mgr), settingsEntity(0) {
        sysName = "SettingsSystem";
    }

    void SettingsSystem::Start() {
        settingsEntity = mgr.AddNewEntity();
        ANI_LOG_INFO("[SettingsSystem] Created settings entity %u", settingsEntity);
    }

    void SettingsSystem::Destroy() {
        if (mgr.IsEntityValid(settingsEntity)) {
            mgr.DestroyEntity(settingsEntity);
            settingsEntity = 0;
        }
        m_settingsComponentIds.clear();
        m_applyCallbacks.clear();
        m_snapshot.clear();
    }

    void SettingsSystem::TrackSettingsComponent(ComponentTypeID id) {
        if (std::find(m_settingsComponentIds.begin(), m_settingsComponentIds.end(), id)
            == m_settingsComponentIds.end()) {
            m_settingsComponentIds.push_back(id);
        }
        BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
        if (comp) {
            m_snapshot[id] = comp->Serialize();
        }
    }

    void SettingsSystem::SetApplyCallback(ComponentTypeID id, std::function<void(BaseComponent&)> cb) {
        m_applyCallbacks[id] = std::move(cb);
    }

    void SettingsSystem::ApplySettings(ComponentTypeID id) {
        auto it = m_applyCallbacks.find(id);
        if (it == m_applyCallbacks.end()) return;
        BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
        if (!comp) return;
        it->second(*comp);
    }

    void SettingsSystem::ApplySettingsByType(const std::string& compName) {
        ComponentTypeID id = mgr.GetComponentTypeIdByName(compName);
        if (id == MAX_COMPONENT_COUNT) return;
        ApplySettings(id);
    }

    FilePathComponent* SettingsSystem::GetFilePathComponent() const {
        if (!mgr.IsEntityValid(settingsEntity)) return nullptr;
        if (!mgr.HasComponent<FilePathComponent>(settingsEntity)) return nullptr;
        return &mgr.GetComponent<FilePathComponent>(settingsEntity);
    }

    std::string SettingsSystem::GetPath(const std::string& key) const {
        auto* c = GetFilePathComponent();
        return c ? c->GetPath(key) : "";
    }

    void SettingsSystem::SetPath(const std::string& key, const std::string& path) {
        auto* c = GetFilePathComponent();
        if (c) c->SetPath(key, path);
    }

    bool SettingsSystem::HasPath(const std::string& key) const {
        auto* c = GetFilePathComponent();
        return c && c->HasPath(key);
    }

    std::vector<std::string> SettingsSystem::GetAllKeys() const {
        auto* c = GetFilePathComponent();
        return c ? c->GetAllKeys() : std::vector<std::string>{};
    }

    void SettingsSystem::RegisterPathDefault(const std::string& key, const std::string& defaultPath) {
        auto* c = GetFilePathComponent();
        if (c) c->SetDefault(key, defaultPath);
    }

    void SettingsSystem::SetMissingPathsCallback(std::function<void(const std::vector<std::string>&)> cb) {
        m_missingPathsCallback = std::move(cb);
    }

    void SettingsSystem::CheckAndPromptMissingPaths() {
        auto* c = GetFilePathComponent();
        if (!c) return;
        std::vector<std::string> missing;
        for (const auto& key : c->GetAllKeys()) {
            if (FilePathComponent::IsPathHidden(key)) continue;
            if (c->GetPath(key).empty()) missing.push_back(key);
        }
        if (missing.empty()) return;
        if (m_missingPathsCallback) m_missingPathsCallback(missing);
    }

    void SettingsSystem::EnsureDirectoriesFromPaths() {
        auto* c = GetFilePathComponent();
        if (!c) return;

        auto isFilePath = [](const std::string& p) {
            static const char* exts[] = { ".json", ".ini", ".log", ".txt", ".cfg", ".toml" };
            for (auto e : exts) {
                size_t n = std::strlen(e);
                if (p.size() >= n && p.compare(p.size() - n, n, e) == 0) return true;
            }
            return false;
            };

        std::error_code ec;
        for (const auto& key : c->GetAllKeys()) {
            if (FilePathComponent::IsPathHidden(key)) continue;
            std::string v = c->GetPath(key);
            if (v.empty()) continue;
            std::filesystem::path p(v);
            if (isFilePath(v)) {
                auto parent = p.parent_path();
                if (!parent.empty()) std::filesystem::create_directories(parent, ec);
            }
            else {
                std::filesystem::create_directories(p, ec);
            }
        }
    }

    std::string SettingsSystem::GetSettingsDirectory() const {
        return "./data/settings";
    }

    std::string SettingsSystem::GetPathsFile() const {
        std::string dataPath = GetPath("DataPath");
        if (dataPath.empty()) dataPath = "./data";
        return dataPath + "/paths.json";
    }

    std::string SettingsSystem::GetComponentFilePath(const std::string& compName) const {
        return GetSettingsDirectory() + "/" + compName + ".json";
    }

    bool SettingsSystem::LoadPathComponent() {
        auto* fc = GetFilePathComponent();
        if (!fc) return false;

        std::string pathsFile = GetPathsFile();
        if (!std::filesystem::exists(pathsFile)) return false;

        try {
            std::ifstream f(pathsFile);
            if (!f.is_open()) return false;
            nlohmann::json j;
            f >> j;
            fc->Deserialize(j);
            ANI_LOG_INFO("[SettingsSystem] Loaded paths from %s", pathsFile.c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[SettingsSystem] Failed to load %s: %s", pathsFile.c_str(), e.what());
            return false;
        }
    }

    bool SettingsSystem::SavePathComponent() {
        auto* fc = GetFilePathComponent();
        if (!fc) return false;

        std::string pathsFile = GetPathsFile();
        try {
            std::filesystem::path p(pathsFile);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
            nlohmann::json j = fc->Serialize();
            std::ofstream f(pathsFile);
            if (!f.is_open()) return false;
            f << j.dump(4);
            ANI_LOG_INFO("[SettingsSystem] Saved paths to %s", pathsFile.c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[SettingsSystem] Failed to save %s: %s", pathsFile.c_str(), e.what());
            return false;
        }
    }

    bool SettingsSystem::LoadAllSettings() {
        if (!mgr.IsEntityValid(settingsEntity)) return false;

        std::string settingsDir = GetSettingsDirectory();
        std::filesystem::create_directories(settingsDir);

        for (ComponentTypeID id : m_settingsComponentIds) {
            std::string name = mgr.GetComponentNameById(id);
            if (name == "FilePath") continue;

            BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
            if (!comp) continue;

            std::string path = GetComponentFilePath(name);
            if (!std::filesystem::exists(path)) continue;
            try {
                std::ifstream f(path);
                if (!f.is_open()) continue;
                nlohmann::json j;
                f >> j;
                comp->Deserialize(j);
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[SettingsSystem] Load failed for %s: %s", name.c_str(), e.what());
            }
        }

        bool hadPathsFile = std::filesystem::exists(GetPathsFile());
        LoadPathComponent();

        if (!hadPathsFile) {
            SavePathComponent();
        }

        ApplyAll();
        TakeSnapshot();
        return true;
    }

    bool SettingsSystem::SaveAllSettings() {
        if (!mgr.IsEntityValid(settingsEntity)) return false;

        std::string settingsDir = GetSettingsDirectory();
        std::filesystem::create_directories(settingsDir);

        bool ok = true;
        for (ComponentTypeID id : m_settingsComponentIds) {
            std::string name = mgr.GetComponentNameById(id);
            if (name == "FilePath") continue;

            BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
            if (!comp) continue;

            std::string path = GetComponentFilePath(name);
            try {
                nlohmann::json j = comp->Serialize();
                std::ofstream f(path);
                if (!f.is_open()) { ok = false; continue; }
                f << j.dump(4);
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[SettingsSystem] Save failed for %s: %s", name.c_str(), e.what());
                ok = false;
            }
        }

        if (!SavePathComponent()) ok = false;

        TakeSnapshot();
        ANI_LOG_INFO("[SettingsSystem] SaveAllSettings %s", ok ? "ok" : "with errors");
        return ok;
    }

    void SettingsSystem::ApplyAll() {
        for (ComponentTypeID id : m_settingsComponentIds) {
            ApplySettings(id);
        }
    }

    void SettingsSystem::TakeSnapshot() {
        m_snapshot.clear();
        for (ComponentTypeID id : m_settingsComponentIds) {
            BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
            if (!comp) continue;
            m_snapshot[id] = comp->Serialize();
        }
    }

    void SettingsSystem::RestoreFromSnapshot() {
        for (const auto& [id, json] : m_snapshot) {
            BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
            if (!comp) continue;
            comp->Deserialize(json);
        }
        ApplyAll();
    }

    bool SettingsSystem::HasUnsavedChanges() const {
        for (ComponentTypeID id : m_settingsComponentIds) {
            BaseComponent* comp = mgr.GetComponentById(settingsEntity, id);
            if (!comp) continue;
            auto it = m_snapshot.find(id);
            if (it == m_snapshot.end()) return true;
            if (comp->Serialize() != it->second) return true;
        }
        return false;
    }

}