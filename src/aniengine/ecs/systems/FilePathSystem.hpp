#pragma once

#include "BaseSystem.hpp"
#include "FilePathComponent.hpp"
#include <string>
#include <vector>
#include <functional>

namespace ECS {
    class FilePathSystem : public BaseSystem {
    public:
        explicit FilePathSystem(EntityManager& mgr);
        virtual ~FilePathSystem() = default;

        virtual void Start() override;
        virtual void Update(float deltaT) override {}
        virtual void Destroy() override;

        std::string GetPath(const std::string& key) const;
        void SetPath(const std::string& key, const std::string& path);
        bool HasPath(const std::string& key) const;
        std::vector<std::string> GetAllKeys() const;

        void SaveToFile(const std::string& filepath) const;
        void LoadFromFile(const std::string& filepath);

        void SetMissingPathsCallback(std::function<void(const std::vector<std::string>&)> callback);
        void CheckAndPromptMissingPaths();

        EntityID GetEntityID() const { return m_filePathEntity; }

    private:
        EntityID m_filePathEntity;
        ComponentTypeID m_componentTypeId;
        FilePathComponent* GetComponent() const;
        std::function<void(const std::vector<std::string>&)> m_missingPathsCallback;
    };
}