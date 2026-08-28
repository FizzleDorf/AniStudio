#include "FilePathSystem.hpp"
#include "EntityManager.hpp"
#include "FilePathComponent.hpp"
#include <fstream>
#include <iostream>

namespace ECS {
    FilePathSystem::FilePathSystem(EntityManager& mgr)
        : BaseSystem(mgr), m_filePathEntity(0), m_componentTypeId(MAX_COMPONENT_COUNT) {
        sysName = "FilePathSystem";
    }

    void FilePathSystem::Start() {
        m_componentTypeId = mgr.RegisterComponent<FilePathComponent>("FilePathComponent");
        m_filePathEntity = mgr.AddNewEntity();
        mgr.AddComponent<FilePathComponent>(m_filePathEntity);
        std::cout << "[FilePathSystem] Started with entity " << m_filePathEntity << std::endl;
    }

    void FilePathSystem::Destroy() {
        if (mgr.IsEntityValid(m_filePathEntity)) {
            mgr.DestroyEntity(m_filePathEntity);
            m_filePathEntity = 0;
        }
    }

    FilePathComponent* FilePathSystem::GetComponent() const {
        if (!mgr.IsEntityValid(m_filePathEntity)) return nullptr;
        auto* base = mgr.GetComponentById(m_filePathEntity, m_componentTypeId);
        return dynamic_cast<FilePathComponent*>(base);
    }

    std::string FilePathSystem::GetPath(const std::string& key) const {
        auto* comp = GetComponent();
        return comp ? comp->GetPath(key) : "";
    }

    void FilePathSystem::SetPath(const std::string& key, const std::string& path) {
        auto* comp = GetComponent();
        if (comp) comp->SetPath(key, path);
    }

    bool FilePathSystem::HasPath(const std::string& key) const {
        auto* comp = GetComponent();
        return comp && comp->HasPath(key);
    }

    std::vector<std::string> FilePathSystem::GetAllKeys() const {
        auto* comp = GetComponent();
        return comp ? comp->GetAllKeys() : std::vector<std::string>{};
    }

    static bool IsPathHidden(const std::string& key) {
        return (key == "CurrentProject" || key == "ProjectData" ||
            key == "ProjectAssets" || key == "ProjectOutput" ||
            key == "Output" || key == "LastOpenProject" || key == "ProjectDataPath");
    }

    void FilePathSystem::SaveToFile(const std::string& filepath) const {
        auto* comp = GetComponent();
        if (!comp) return;

        nlohmann::json j;
        j["compName"] = comp->compName;

        nlohmann::json pathsJson;
        const auto& allPaths = comp->GetPathMap();
        for (const auto& [key, value] : allPaths) {
            if (IsPathHidden(key)) {
                continue;
            }
            pathsJson[key] = value;
        }
        j["paths"] = pathsJson;

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << j.dump(4);
        }
    }

    void FilePathSystem::LoadFromFile(const std::string& filepath) {
        if (!std::filesystem::exists(filepath)) return;
        std::ifstream file(filepath);
        if (!file.is_open()) return;
        nlohmann::json j;
        file >> j;
        auto* comp = GetComponent();
        if (comp) comp->Deserialize(j);
    }

    void FilePathSystem::SetMissingPathsCallback(std::function<void(const std::vector<std::string>&)> callback) {
        m_missingPathsCallback = callback;
    }

    void FilePathSystem::CheckAndPromptMissingPaths() {
        auto* comp = GetComponent();
        if (!comp) return;
        std::vector<std::string> missing;
        for (const auto& key : comp->GetAllKeys()) {
            if (IsPathHidden(key)) continue;
            if (comp->GetPath(key).empty()) {
                missing.push_back(key);
            }
        }
        if (!missing.empty() && m_missingPathsCallback) {
            m_missingPathsCallback(missing);
        }
    }
}