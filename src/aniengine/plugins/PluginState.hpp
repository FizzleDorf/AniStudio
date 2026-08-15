#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace ECS {
    class EntityManager;
}

namespace Plugins {

    struct PluginLoadState {
        bool loaded = false;
        bool enabled = false;
        std::string path;
        uint32_t version = 0;

        nlohmann::json Serialize() const;
        void Deserialize(const nlohmann::json& j);
    };

    class PluginState {
    public:
        PluginState();
        ~PluginState() = default;

        void SetEntityManager(ECS::EntityManager* mgr);
        void SetCurrentProjectPath(const std::string& projectPath);
        bool LoadProjectPluginState();
        bool SaveProjectPluginState();

        bool ShouldLoadPlugin(const std::string& pluginName) const;
        bool ShouldEnablePlugin(const std::string& pluginName) const;
        std::unordered_set<std::string> GetPluginsToLoad() const;
        std::unordered_set<std::string> GetPluginsToEnable() const;
        std::string GetPluginPath(const std::string& pluginName) const;
        std::unordered_map<std::string, PluginLoadState> GetAllPluginStates() const;

        void SetPluginState(const std::string& pluginName, bool loaded, bool enabled, const std::string& path = "", uint32_t version = 0);
        void RemovePluginState(const std::string& pluginName);

        void DebugPrintState() const;

    private:
        ECS::EntityManager* m_entityManager = nullptr;
        std::string m_currentProjectPath;

        std::unordered_map<std::string, PluginLoadState> m_projectPluginState;

        bool SaveStateToFile(const std::string& filepath, const std::unordered_map<std::string, PluginLoadState>& state);
        bool LoadStateFromFile(const std::string& filepath, std::unordered_map<std::string, PluginLoadState>& state);
        std::string GetProjectPluginStatePath() const;
    };

} // namespace Plugins