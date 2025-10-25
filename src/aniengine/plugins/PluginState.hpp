#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

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

		// Global state management
		void SetGlobalDataPath(const std::string& dataPath);
		bool LoadGlobalPluginState();
		bool SaveGlobalPluginState();

		// Project state management
		void SetCurrentProjectPath(const std::string& projectPath);
		bool LoadProjectPluginState();
		bool SaveProjectPluginState();

		// State queries
		bool ShouldLoadPlugin(const std::string& pluginName) const;
		bool ShouldEnablePlugin(const std::string& pluginName) const;
		std::unordered_set<std::string> GetPluginsToLoad() const;
		std::unordered_set<std::string> GetPluginsToEnable() const;

		// FIXED: Get plugin path from state
		std::string GetPluginPath(const std::string& pluginName) const;
		std::unordered_map<std::string, PluginLoadState> GetAllPluginStates() const;

		// State updates (called by PluginManager)
		void SetPluginState(const std::string& pluginName, bool loaded, bool enabled, const std::string& path = "", uint32_t version = 0);
		void RemovePluginState(const std::string& pluginName);

		// Mode switching
		void UseGlobalState();
		void UseProjectState();
		bool IsUsingProjectState() const { return usingProjectState; }

		// Debug
		void DebugPrintState() const;

	private:
		std::string globalDataPath;
		std::string currentProjectPath;
		bool usingProjectState = false;

		// Global and project plugin states
		std::unordered_map<std::string, PluginLoadState> globalPluginState;
		std::unordered_map<std::string, PluginLoadState> projectPluginState;

		// Active state reference (points to either global or project)
		std::unordered_map<std::string, PluginLoadState>* activeState = nullptr;

		// Helper methods
		std::string GetGlobalPluginStatePath() const;
		std::string GetProjectPluginStatePath() const;
		void UpdateActiveStateReference();

		// File operations
		bool SaveStateToFile(const std::string& filepath, const std::unordered_map<std::string, PluginLoadState>& state);
		bool LoadStateFromFile(const std::string& filepath, std::unordered_map<std::string, PluginLoadState>& state);
	};

} // namespace Plugins