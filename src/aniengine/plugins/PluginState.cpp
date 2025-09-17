#include "PluginState.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

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
		activeState = &globalPluginState;
	}

	void PluginState::SetGlobalDataPath(const std::string& dataPath) {
		globalDataPath = dataPath;
		std::cout << "[PluginState] Global data path set to: " << dataPath << std::endl;
	}

	void PluginState::SetCurrentProjectPath(const std::string& projectPath) {
		currentProjectPath = projectPath;
		std::cout << "[PluginState] Current project path set to: " << projectPath << std::endl;
	}

	std::string PluginState::GetGlobalPluginStatePath() const {
		return globalDataPath + "/plugin_state.json";
	}

	std::string PluginState::GetProjectPluginStatePath() const {
		if (currentProjectPath.empty()) return "";
		return currentProjectPath + "/plugin_state.json";
	}

	void PluginState::UpdateActiveStateReference() {
		if (usingProjectState) {
			activeState = &projectPluginState;
		}
		else {
			activeState = &globalPluginState;
		}
	}

	void PluginState::UseGlobalState() {
		usingProjectState = false;
		UpdateActiveStateReference();
		std::cout << "[PluginState] Switched to global plugin state" << std::endl;
	}

	void PluginState::UseProjectState() {
		usingProjectState = true;
		UpdateActiveStateReference();
		std::cout << "[PluginState] Switched to project plugin state" << std::endl;
	}

	bool PluginState::LoadGlobalPluginState() {
		std::string filePath = GetGlobalPluginStatePath();

		if (!std::filesystem::exists(filePath)) {
			std::cout << "[PluginState] No global plugin state file found, starting fresh" << std::endl;
			return true; // Not an error, just no saved state
		}

		bool success = LoadStateFromFile(filePath, globalPluginState);
		if (success) {
			std::cout << "[PluginState] Global plugin state loaded: " << globalPluginState.size() << " plugins" << std::endl;
		}
		return success;
	}

	bool PluginState::SaveGlobalPluginState() {
		std::string filePath = GetGlobalPluginStatePath();

		// Ensure directory exists
		std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

		bool success = SaveStateToFile(filePath, globalPluginState);
		if (success) {
			std::cout << "[PluginState] Global plugin state saved: " << globalPluginState.size() << " plugins" << std::endl;
		}
		return success;
	}

	bool PluginState::LoadProjectPluginState() {
		std::string filePath = GetProjectPluginStatePath();

		if (filePath.empty() || !std::filesystem::exists(filePath)) {
			std::cout << "[PluginState] No project plugin state file found" << std::endl;
			projectPluginState.clear(); // Start with empty project state
			return true;
		}

		bool success = LoadStateFromFile(filePath, projectPluginState);
		if (success) {
			std::cout << "[PluginState] Project plugin state loaded: " << projectPluginState.size() << " plugins" << std::endl;
		}
		return success;
	}

	bool PluginState::SaveProjectPluginState() {
		std::string filePath = GetProjectPluginStatePath();

		if (filePath.empty()) {
			std::cerr << "[PluginState] Cannot save project state - no project path set" << std::endl;
			return false;
		}

		// Ensure directory exists
		std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

		bool success = SaveStateToFile(filePath, projectPluginState);
		if (success) {
			std::cout << "[PluginState] Project plugin state saved: " << projectPluginState.size() << " plugins" << std::endl;
		}
		return success;
	}

	bool PluginState::SaveStateToFile(const std::string& filepath, const std::unordered_map<std::string, PluginLoadState>& state) {
		try {
			nlohmann::json j;
			j["version"] = "1.0";
			j["plugins"] = nlohmann::json::object();

			for (const auto&[pluginName, pluginState] : state) {
				j["plugins"][pluginName] = pluginState.Serialize();
			}

			std::ofstream file(filepath);
			if (!file.is_open()) {
				std::cerr << "[PluginState] Failed to open file for writing: " << filepath << std::endl;
				return false;
			}

			file << j.dump(4);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginState] Failed to save plugin state: " << e.what() << std::endl;
			return false;
		}
	}

	bool PluginState::LoadStateFromFile(const std::string& filepath, std::unordered_map<std::string, PluginLoadState>& state) {
		try {
			std::ifstream file(filepath);
			if (!file.is_open()) {
				std::cerr << "[PluginState] Failed to open file for reading: " << filepath << std::endl;
				return false;
			}

			nlohmann::json j;
			file >> j;

			state.clear();

			if (j.contains("plugins") && j["plugins"].is_object()) {
				for (const auto&[pluginName, pluginData] : j["plugins"].items()) {
					PluginLoadState pluginState;
					pluginState.Deserialize(pluginData);
					state[pluginName] = pluginState;
				}
			}

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginState] Failed to load plugin state: " << e.what() << std::endl;
			return false;
		}
	}

	bool PluginState::ShouldLoadPlugin(const std::string& pluginName) const {
		if (!activeState) return false;

		auto it = activeState->find(pluginName);
		if (it == activeState->end()) return false;

		return it->second.loaded;
	}

	bool PluginState::ShouldEnablePlugin(const std::string& pluginName) const {
		if (!activeState) return false;

		auto it = activeState->find(pluginName);
		if (it == activeState->end()) return false;

		return it->second.loaded && it->second.enabled;
	}

	std::unordered_set<std::string> PluginState::GetPluginsToLoad() const {
		std::unordered_set<std::string> result;

		if (activeState) {
			for (const auto&[pluginName, state] : *activeState) {
				if (state.loaded) {
					result.insert(pluginName);
				}
			}
		}

		return result;
	}

	std::unordered_set<std::string> PluginState::GetPluginsToEnable() const {
		std::unordered_set<std::string> result;

		if (activeState) {
			for (const auto&[pluginName, state] : *activeState) {
				if (state.loaded && state.enabled) {
					result.insert(pluginName);
				}
			}
		}

		return result;
	}

	void PluginState::SetPluginState(const std::string& pluginName, bool loaded, bool enabled, const std::string& path, uint32_t version) {
		if (!activeState) return;

		PluginLoadState& state = (*activeState)[pluginName];
		state.loaded = loaded;
		state.enabled = enabled;
		if (!path.empty()) state.path = path;
		if (version > 0) state.version = version;

		std::cout << "[PluginState] Updated state for " << pluginName
			<< " - loaded: " << loaded << ", enabled: " << enabled << std::endl;
	}

	void PluginState::RemovePluginState(const std::string& pluginName) {
		if (!activeState) return;

		activeState->erase(pluginName);
		std::cout << "[PluginState] Removed state for " << pluginName << std::endl;
	}

	void PluginState::DebugPrintState() const {
		std::cout << "[PluginState] Current state (using " << (usingProjectState ? "project" : "global") << "):" << std::endl;

		if (activeState) {
			for (const auto&[pluginName, state] : *activeState) {
				std::cout << "  " << pluginName
					<< " - loaded: " << state.loaded
					<< ", enabled: " << state.enabled
					<< ", path: " << state.path
					<< ", version: " << state.version << std::endl;
			}
		}
	}

} // namespace Plugins