#pragma once
#include "BaseSystem.hpp"
#include "FilePathComponent.hpp"
#include <string>
#include <vector>

namespace ECS {
    class FilePathSystem : public BaseSystem {
    public:
        explicit FilePathSystem(EntityManager& mgr);
        virtual ~FilePathSystem() = default;

        virtual void Start() override;
        virtual void Update(float deltaT) override {}
        virtual void Destroy() override;

        // Generic key-based access
        std::string GetPath(const std::string& key) const;
        void SetPath(const std::string& key, const std::string& path);
        bool HasPath(const std::string& key) const;
        std::vector<std::string> GetAllKeys() const;

        // Persistence
        void SaveToFile(const std::string& filepath) const;
        void LoadFromFile(const std::string& filepath);

    private:
        EntityID m_filePathEntity;
        ComponentTypeID m_componentTypeId;
        FilePathComponent* GetComponent() const;
    };
} //namespace ECS