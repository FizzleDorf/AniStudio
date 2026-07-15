#pragma once

#include "BaseComponent.hpp"
#include <unordered_map>

namespace ECS {

    class FilePathComponent : public BaseComponent {
    public:
        FilePathComponent() { compName = "FilePathComponent"; }

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

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["compName"] = compName;
            j["paths"] = m_paths;
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            if (j.contains("compName")) compName = j["compName"];
            if (j.contains("paths") && j["paths"].is_object()) {
                for (auto& [key, value] : j["paths"].items()) {
                    if (value.is_string()) m_paths[key] = value.get<std::string>();
                }
            }
        }

    private:
        std::unordered_map<std::string, std::string> m_paths;
    };

} // namespace ECS