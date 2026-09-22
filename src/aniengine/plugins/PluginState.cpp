#include "PluginState.hpp"
#include "FilePathSystem.hpp"
#include "EntityManager.hpp"
#include "Log.hpp"

#include <filesystem>
#include <fstream>

namespace Plugins {

    nlohmann::json PluginLoadState::Serialize() const {
        return {
            {"loaded", loaded},
            {"enabled", enabled},
            {"path", path},
            {"version", version}
        };
    }

    void PluginLoadState::Deserialize(const nlohmann::json& j) {
        if (j.contains("loaded")) loaded = j["loaded"];
        if (j.contains("enabled")) enabled = j["enabled"];
        if (j.contains("path")) path = j["path"];
        if (j.contains("version")) version = j["version"];
    }

    PluginState::PluginState() {
        ANI_LOG_DEBUG("[PluginState] Created");
    }

    void PluginState::SetEntityManager(ECS::EntityManager* mgr) {
        m_entityManager = mgr;
    }

    void PluginState::SetCurrentProjectPath(const std::string& projectPath) {
        m_currentProjectPath = projectPath;
        ANI_LOG_INFO("[PluginState] Current project path set to: %s", projectPath.c_str());
    }

    std::string PluginState::GetProjectPluginStatePath() const {
        if (m_currentProjectPath.empty()) {
            ANI_LOG_WARN("[PluginState] No project path set");
            return "";
        }

        auto fs = m_entityManager ? m_entityManager->GetSystem<ECS::FilePathSystem>() : nullptr;
        std::string dataPath;
        if (fs) {
            dataPath = fs->GetPath("ProjectData");
        }
        if (dataPath.empty()) {
            dataPath = m_currentProjectPath + "/data";
        }

        return dataPath + "/plugin_state.json";
    }

    bool PluginState::LoadProjectPluginState() {
        std::string filePath = GetProjectPluginStatePath();

        if (filePath.empty() || !std::filesystem::exists(filePath)) {
            ANI_LOG_DEBUG("[PluginState] No project plugin state file found, starting fresh");
            m_projectPluginState.clear();
            return true;
        }

        bool success = LoadStateFromFile(filePath, m_projectPluginState);
        if (success) {
            ANI_LOG_INFO("[PluginState] Project plugin state loaded: %zu plugins",
                m_projectPluginState.size());
        }
        return success;
    }

    bool PluginState::SaveProjectPluginState() {
        std::string filePath = GetProjectPluginStatePath();

        if (filePath.empty()) {
            ANI_LOG_WARN("[PluginState] Cannot save project state - no project path set");
            return false;
        }

        std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

        bool success = SaveStateToFile(filePath, m_projectPluginState);
        if (success) {
            ANI_LOG_INFO("[PluginState] Project plugin state saved: %zu plugins",
                m_projectPluginState.size());
        }
        return success;
    }

    bool PluginState::SaveStateToFile(const std::string& filepath, const std::unordered_map<std::string, PluginLoadState>& state) {
        try {
            nlohmann::json j;
            j["version"] = "1.0";
            j["plugins"] = nlohmann::json::object();

            for (const auto& [pluginName, pluginState] : state) {
                j["plugins"][pluginName] = pluginState.Serialize();
            }

            std::ofstream file(filepath);
            if (!file.is_open()) {
                ANI_LOG_WARN("[PluginState] Failed to open file for writing: %s", filepath.c_str());
                return false;
            }

            file << j.dump(4);
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[PluginState] Failed to save plugin state: %s", e.what());
            return false;
        }
    }

    bool PluginState::LoadStateFromFile(const std::string& filepath, std::unordered_map<std::string, PluginLoadState>& state) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                ANI_LOG_WARN("[PluginState] Failed to open file for reading: %s", filepath.c_str());
                return false;
            }

            nlohmann::json j;
            file >> j;

            state.clear();

            if (j.contains("plugins") && j["plugins"].is_object()) {
                for (const auto& [pluginName, pluginData] : j["plugins"].items()) {
                    PluginLoadState pluginState;
                    pluginState.Deserialize(pluginData);
                    state[pluginName] = pluginState;
                }
            }

            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[PluginState] Failed to load plugin state: %s", e.what());
            return false;
        }
    }

    bool PluginState::ShouldLoadPlugin(const std::string& pluginName) const {
        auto it = m_projectPluginState.find(pluginName);
        if (it == m_projectPluginState.end()) return false;
        return it->second.loaded;
    }

    bool PluginState::ShouldEnablePlugin(const std::string& pluginName) const {
        auto it = m_projectPluginState.find(pluginName);
        if (it == m_projectPluginState.end()) return false;
        return it->second.loaded && it->second.enabled;
    }

    std::unordered_set<std::string> PluginState::GetPluginsToLoad() const {
        std::unordered_set<std::string> result;
        for (const auto& [pluginName, state] : m_projectPluginState) {
            if (state.loaded) {
                result.insert(pluginName);
            }
        }
        return result;
    }

    std::unordered_set<std::string> PluginState::GetPluginsToEnable() const {
        std::unordered_set<std::string> result;
        for (const auto& [pluginName, state] : m_projectPluginState) {
            if (state.loaded && state.enabled) {
                result.insert(pluginName);
            }
        }
        return result;
    }

    std::string PluginState::GetPluginPath(const std::string& pluginName) const {
        auto it = m_projectPluginState.find(pluginName);
        if (it == m_projectPluginState.end()) return "";
        return it->second.path;
    }

    std::unordered_map<std::string, PluginLoadState> PluginState::GetAllPluginStates() const {
        return m_projectPluginState;
    }

    void PluginState::SetPluginState(const std::string& pluginName, bool loaded, bool enabled, const std::string& path, uint32_t version) {
        PluginLoadState& state = m_projectPluginState[pluginName];
        state.loaded = loaded;
        state.enabled = enabled;
        if (!path.empty()) state.path = path;
        if (version > 0) state.version = version;

        ANI_LOG_TRACE("[PluginState] Updated state for %s - loaded: %s, enabled: %s, path: %s",
            pluginName.c_str(),
            loaded ? "true" : "false",
            enabled ? "true" : "false",
            path.c_str());
    }

    void PluginState::RemovePluginState(const std::string& pluginName) {
        m_projectPluginState.erase(pluginName);
        ANI_LOG_DEBUG("[PluginState] Removed state for %s", pluginName.c_str());
    }

    void PluginState::DebugPrintState() const {
        ANI_LOG_DEBUG("[PluginState] Current project plugin state:");
        for (const auto& [pluginName, state] : m_projectPluginState) {
            ANI_LOG_DEBUG("  %s - loaded: %s, enabled: %s, path: %s, version: %u",
                pluginName.c_str(),
                state.loaded ? "true" : "false",
                state.enabled ? "true" : "false",
                state.path.c_str(),
                state.version);
        }
    }

} // namespace Plugins