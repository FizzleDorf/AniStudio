#pragma once
#include "BaseComponent.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <vector>

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

        std::string GetPath(const std::string& key) const {
            auto it = m_current.find(key);
            return (it != m_current.end()) ? it->second : "";
        }

        std::string GetDefaultPath(const std::string& key) const {
            auto it = m_defaults.find(key);
            return (it != m_defaults.end()) ? it->second : "";
        }

        bool HasPath(const std::string& key) const {
            return m_current.find(key) != m_current.end() ||
                m_defaults.find(key) != m_defaults.end();
        }

        bool HasDefault(const std::string& key) const {
            return m_defaults.find(key) != m_defaults.end();
        }

        void SetPath(const std::string& key, const std::string& path) {
            m_current[key] = path;
        }

        void SetDefault(const std::string& key, const std::string& defaultPath) {
            m_defaults[key] = defaultPath;
            if (m_current.find(key) == m_current.end()) {
                m_current[key] = "";
            }
        }

        std::vector<std::string> GetAllKeys() const {
            std::vector<std::string> keys;
            keys.reserve(m_defaults.size() + m_current.size());
            for (const auto& [k, _] : m_defaults) keys.push_back(k);
            for (const auto& [k, _] : m_current) {
                if (m_defaults.find(k) == m_defaults.end()) keys.push_back(k);
            }
            return keys;
        }

        const std::unordered_map<std::string, std::string>& GetCurrentMap() const {
            return m_current;
        }

        const std::unordered_map<std::string, std::string>& GetDefaultMap() const {
            return m_defaults;
        }

        nlohmann::json Serialize() const override {
            nlohmann::json paths = nlohmann::json::object();
            for (const auto& key : GetAllKeys()) {
                paths[key] = GetPath(key);
            }
            nlohmann::json j;
            j[GetCompName()] = { {"paths", paths} };
            return j;
        }

        void Deserialize(const nlohmann::json& j) override {
            const char* key = GetCompName();
            nlohmann::json data;
            if (j.contains(key)) data = j.at(key);
            else                 data = j;

            if (data.contains("paths") && data["paths"].is_object()) {
                for (auto& [k, v] : data["paths"].items()) {
                    if (v.is_string()) m_current[k] = v.get<std::string>();
                }
            }
        }

        static bool IsPathHidden(const std::string& key) {
            return (key == "CurrentProject" || key == "ProjectData" ||
                key == "ProjectAssets" || key == "ProjectOutput" ||
                key == "Output" || key == "LastOpenProject" ||
                key == "ProjectDataPath" || key == "AssetsFolder" ||
                key == "Assets" || key == "OutputFolder");
        }

    private:
        std::unordered_map<std::string, std::string> m_current;
        std::unordered_map<std::string, std::string> m_defaults;
    };

}