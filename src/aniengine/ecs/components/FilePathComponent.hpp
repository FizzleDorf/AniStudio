#pragma once

#include "BaseComponent.hpp"
#include <unordered_map>

namespace ECS {

    class FilePathComponent : public BaseComponent {
    public:
        FilePathComponent() = default;

        const char* GetCompName() const override { return "FilePathComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = nlohmann::json::object();
            return j;
        }

        FilePathComponent(const FilePathComponent& other)
            : BaseComponent(other)
            , m_paths(other.m_paths) {
        }

        FilePathComponent& operator=(const FilePathComponent& other) {
            if (this != &other) {
                m_paths = other.m_paths;
            }
            return *this;
        }

        std::string GetPath(const std::string& key) const {
            auto it = m_paths.find(key);
            return (it != m_paths.end()) ? it->second : "";
        }

        void SetPath(const std::string& key, const std::string& path) {
            m_paths[key] = path;
        }

        bool HasPath(const std::string& key) const {
            return m_paths.find(key) != m_paths.end();
        }

        std::vector<std::string> GetAllKeys() const {
            std::vector<std::string> keys;
            keys.reserve(m_paths.size());
            for (const auto& [k, _] : m_paths) keys.push_back(k);
            return keys;
        }

        const std::unordered_map<std::string, std::string>& GetPathMap() const {
            return m_paths;
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j[GetCompName()] = {
                {"paths", m_paths}
            };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json componentData;
            if (j.contains(key))
                componentData = j.at(key);
            else
                componentData = j;

            if (componentData.contains("paths") && componentData["paths"].is_object()) {
                for (auto& [pathKey, value] : componentData["paths"].items()) {
                    if (value.is_string()) m_paths[pathKey] = value.get<std::string>();
                }
            }
        }

    private:
        std::unordered_map<std::string, std::string> m_paths;
    };

}