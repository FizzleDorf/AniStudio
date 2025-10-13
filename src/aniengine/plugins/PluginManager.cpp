#include "PluginManager.hpp"
#include "EntityManager.hpp"
#include "PluginState.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Plugins {

	PluginManager::PluginManager(ECS::EntityManager& entityMgr)
		: entityManager(entityMgr) {
		std::cout << "[PluginManager] Constructor - simplified manager created" << std::endl;

		std::filesystem::path executablePath = std::filesystem::current_path();
		stagingDirectory = (executablePath / "plugins").string();
		std::filesystem::create_directories(stagingDirectory);
		std::cout << "[PluginManager] Base plugins directory set to: " << stagingDirectory << std::endl;

		InitializePluginStateManager();

		hotReloadEnabled = false;
		hotReloadForced = false;
		std::cout << "[PluginManager] Hot reload disabled by default" << std::endl;
	}

	PluginManager::~PluginManager() {
		std::cout << "[PluginManager] Destructor - cleaning up plugins..." << std::endl;
		hotReloadEnabled = false;

		std::vector<std::string> pluginNames;
		for (const auto& pair : plugins) {
			pluginNames.push_back(pair.first);
		}

		for (const auto& name : pluginNames) {
			if (plugins[name].loaded) {
				if (plugins[name].enabled) disablePlugin(name);
				unloadPlugin(name);
			}
		}
	}

	void PluginManager::InitializePluginStateManager() {
		pluginState = std::make_unique<PluginState>();
		std::cout << "[PluginManager] Plugin state manager created" << std::endl;
	}

	void PluginManager::SetGlobalDataPath(const std::string& dataPath) {
		if (pluginState) {
			pluginState->SetGlobalDataPath(dataPath);
		}
	}

	void PluginManager::LoadGlobalPluginState() {
		if (!pluginState) return;

		std::cout << "[PluginManager] Loading global plugin state..." << std::endl;
		pluginState->UseGlobalState();
		pluginState->LoadGlobalPluginState();

		LoadPluginsFromState();

		std::cout << "[PluginManager] Global plugin state loaded and applied" << std::endl;
	}

	void PluginManager::SaveGlobalPluginState() {
		if (!pluginState) return;

		std::cout << "[PluginManager] Saving global plugin state..." << std::endl;
		SaveCurrentPluginState();
		pluginState->SaveGlobalPluginState();
		std::cout << "[PluginManager] Global plugin state saved" << std::endl;
	}

	void PluginManager::SetProjectContext(const std::string& projectPath) {
		if (!pluginState) return;

		std::cout << "[PluginManager] Setting project context: " << projectPath << std::endl;

		SaveCurrentPluginState();

		pluginState->SetCurrentProjectPath(projectPath);
		pluginState->UseProjectState();
		pluginState->LoadProjectPluginState();

		LoadPluginsFromState();

		std::cout << "[PluginManager] Project plugin context applied" << std::endl;
	}

	void PluginManager::SaveProjectPluginState() {
		if (!pluginState) return;

		std::cout << "[PluginManager] Saving project plugin state..." << std::endl;
		SaveCurrentPluginState();
		pluginState->SaveProjectPluginState();
		std::cout << "[PluginManager] Project plugin state saved" << std::endl;
	}

	void PluginManager::UseGlobalPluginState() {
		if (!pluginState) return;

		std::cout << "[PluginManager] Reverting to global plugin state..." << std::endl;

		SaveCurrentPluginState();

		pluginState->UseGlobalState();

		LoadPluginsFromState();

		std::cout << "[PluginManager] Global plugin state restored" << std::endl;
	}

	void PluginManager::LoadPluginsFromState() {
		if (!pluginState) return;

		auto pluginsToLoad = pluginState->GetPluginsToLoad();
		auto pluginsToEnable = pluginState->GetPluginsToEnable();

		std::cout << "[PluginManager] Loading " << pluginsToLoad.size() << " plugins from saved state" << std::endl;

		std::vector<std::string> currentPlugins;
		for (const auto&[name, info] : plugins) {
			currentPlugins.push_back(name);
		}

		for (const auto& name : currentPlugins) {
			if (plugins[name].enabled) {
				disablePlugin(name);
			}
			unloadPlugin(name);
		}

		for (const auto& pluginName : pluginsToLoad) {
			std::string pluginPath = stagingDirectory + "/" + pluginName;
			if (std::filesystem::exists(pluginPath)) {
				std::cout << "[PluginManager] Loading plugin from state: " << pluginName << std::endl;
				if (loadPlugin(pluginPath)) {
					if (pluginsToEnable.find(pluginName) != pluginsToEnable.end()) {
						std::cout << "[PluginManager] Enabling plugin from state: " << pluginName << std::endl;
						enablePlugin(pluginName);
					}
				}
			}
			else {
				std::cout << "[PluginManager] Warning: Plugin path not found for " << pluginName << ": " << pluginPath << std::endl;
			}
		}
	}

	void PluginManager::SaveCurrentPluginState() {
		if (!pluginState) return;

		for (const auto&[pluginName, info] : plugins) {
			pluginState->SetPluginState(
				pluginName,
				info.loaded,
				info.enabled,
				info.path,
				info.currentVersion
			);
		}
	}

	void PluginManager::setStagingDirectory(const std::string& basePluginsDir) {
		std::filesystem::path absPath = std::filesystem::absolute(basePluginsDir);
		stagingDirectory = absPath.string();
		std::filesystem::create_directories(stagingDirectory);
		std::cout << "[PluginManager] Base plugins directory changed to: " << stagingDirectory << std::endl;
	}

	void PluginManager::enableHotReload(bool enable) {
		hotReloadEnabled = enable;
		std::cout << "[PluginManager] Hot reload " << (enable ? "enabled" : "disabled") << std::endl;
		if (enable) {
			std::filesystem::create_directories(stagingDirectory);
		}
	}

	void PluginManager::setHotReloadForce(bool force) {
		hotReloadForced = force;
		if (force) {
			hotReloadEnabled = true;
			std::cout << "[PluginManager] Hot reload FORCE ENABLED (for development)" << std::endl;
			std::filesystem::create_directories(stagingDirectory);
		}
		else {
			std::cout << "[PluginManager] Hot reload force disabled" << std::endl;
		}
	}

	void PluginManager::setupPluginDirectories(const std::string& pluginName) {
		std::string pluginMainDir = stagingDirectory + "/" + pluginName;
		std::string pluginStagingDir = pluginMainDir + "/staging";

		std::filesystem::create_directories(pluginMainDir);
		std::filesystem::create_directories(pluginStagingDir);

		std::cout << "[PluginManager] Setup directories for plugin: " << pluginName << std::endl;
		std::cout << "  Main: " << pluginMainDir << std::endl;
		std::cout << "  Staging: " << pluginStagingDir << std::endl;
	}

	std::string PluginManager::findPluginDll(const std::string& pluginDir, const std::string& pluginName) {
		std::vector<std::string> possibleNames = {
			pluginName + ".dll",
			pluginName + ".so",
			"lib" + pluginName + ".so"
		};

		for (const auto& dllName : possibleNames) {
			std::string dllPath = pluginDir + "/" + dllName;
			if (std::filesystem::exists(dllPath)) {
				return dllPath;
			}
		}
		return "";
	}

	std::string PluginManager::getVersionedDllName(const std::string& pluginName, uint32_t version) {
		return pluginName + "_v" + std::to_string(version) + ".dll";
	}

	std::string PluginManager::findNewestVersionedDll(const std::string& pluginDir, const std::string& pluginName) {
		if (!std::filesystem::exists(pluginDir)) {
			return "";
		}

		std::string newestDll = "";
		uint32_t highestVersion = 0;

		try {
			for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
				if (entry.is_regular_file()) {
					std::string filename = entry.path().filename().string();
					uint32_t version = extractVersionFromDllName(entry.path().string(), pluginName);

					if (version > 0 && version > highestVersion) {
						highestVersion = version;
						newestDll = entry.path().string();
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Error scanning for versioned DLLs: " << e.what() << std::endl;
		}

		return newestDll;
	}

	uint32_t PluginManager::extractVersionFromDllName(const std::string& dllPath, const std::string& pluginName) {
		std::filesystem::path path(dllPath);
		std::string filename = path.filename().string();

		std::string pattern = pluginName + "_v(\\d+)\\.dll";
		std::regex versionRegex(pattern);
		std::smatch match;

		if (std::regex_search(filename, match, versionRegex)) {
			return static_cast<uint32_t>(std::stoul(match[1].str()));
		}

		return 0;
	}

	void PluginManager::cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return;

		PluginInfo& plugin = it->second;
		std::string pluginMainDir = stagingDirectory + "/" + pluginName;

		if (!std::filesystem::exists(pluginMainDir)) return;

		std::vector<std::pair<uint32_t, std::string>> versionedDlls;

		try {
			for (const auto& entry : std::filesystem::directory_iterator(pluginMainDir)) {
				if (entry.is_regular_file()) {
					uint32_t version = extractVersionFromDllName(entry.path().string(), pluginName);
					if (version > 0) {
						versionedDlls.emplace_back(version, entry.path().string());
					}
				}
			}

			std::sort(versionedDlls.begin(), versionedDlls.end(),
				[](const auto& a, const auto& b) { return a.first > b.first; });

			for (size_t i = keepVersionsCount; i < versionedDlls.size(); ++i) {
				const auto&[version, dllPath] = versionedDlls[i];

				if (dllPath != plugin.activeDllPath) {
					try {
						std::filesystem::remove(dllPath);
						std::cout << "[PluginManager] Cleaned up old DLL version " << version
							<< ": " << dllPath << std::endl;
					}
					catch (const std::exception& e) {
						std::cerr << "[PluginManager] Failed to delete old DLL " << dllPath
							<< ": " << e.what() << std::endl;
					}
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Error during cleanup: " << e.what() << std::endl;
		}
	}

	bool PluginManager::loadPlugin(const std::string& pluginDirPath) {
		std::filesystem::path pluginPath(pluginDirPath);

		if (!std::filesystem::exists(pluginPath) || !std::filesystem::is_directory(pluginPath)) {
			std::cerr << "[PluginManager] Invalid plugin directory: " << pluginDirPath << std::endl;
			return false;
		}

		std::string pluginName = pluginPath.filename().string();
		std::cout << "[PluginManager] Loading plugin: " << pluginName << " from: " << pluginDirPath << std::endl;

		if (plugins.find(pluginName) != plugins.end()) {
			std::cout << "[PluginManager] Plugin already loaded: " << pluginName << std::endl;
			return true;
		}

		std::string pluginMainDir = pluginDirPath;
		std::string pluginStagingDir = pluginMainDir + "/staging";

		std::filesystem::create_directories(pluginStagingDir);

		std::string newestVersionedDll = findNewestVersionedDll(pluginMainDir, pluginName);
		std::string sourceDllPath = findPluginDll(pluginDirPath, pluginName);
		std::string stagingDllPath = findPluginDll(pluginStagingDir, pluginName);

		std::string dllToLoad;
		uint32_t currentVersion = 0;

		if (!newestVersionedDll.empty()) {
			dllToLoad = newestVersionedDll;
			currentVersion = extractVersionFromDllName(newestVersionedDll, pluginName);
			std::cout << "[PluginManager] Loading newest versioned DLL v" << currentVersion
				<< ": " << newestVersionedDll << std::endl;
		}
		else if (!stagingDllPath.empty()) {
			currentVersion = 1;
			std::string versionedDllName = getVersionedDllName(pluginName, currentVersion);
			dllToLoad = pluginMainDir + "/" + versionedDllName;

			if (copyFile(stagingDllPath, dllToLoad)) {
				std::filesystem::remove(stagingDllPath);
				std::cout << "[PluginManager] Created first versioned DLL v" << currentVersion
					<< " from staging" << std::endl;
			}
			else {
				std::cerr << "[PluginManager] Failed to create versioned DLL from staging" << std::endl;
				return false;
			}
		}
		else if (!sourceDllPath.empty()) {
			currentVersion = 1;
			std::string versionedDllName = getVersionedDllName(pluginName, currentVersion);
			dllToLoad = pluginMainDir + "/" + versionedDllName;

			if (copyFile(sourceDllPath, dllToLoad)) {
				std::cout << "[PluginManager] Created first versioned DLL v" << currentVersion
					<< " from source" << std::endl;
			}
			else {
				std::cerr << "[PluginManager] Failed to create versioned DLL from source" << std::endl;
				return false;
			}
		}
		else {
			std::cerr << "[PluginManager] No DLL found for plugin: " << pluginName << std::endl;
			std::cerr << "[PluginManager] Searched in:" << std::endl;
			std::cerr << "  Main dir: " << pluginMainDir << std::endl;
			std::cerr << "  Staging dir: " << pluginStagingDir << std::endl;
			return false;
		}

		void* handle = loadDynamicLibrary(dllToLoad);
		if (!handle) {
			std::cerr << "[PluginManager] Failed to load library: " << dllToLoad << std::endl;
			return false;
		}

		auto createFunc = reinterpret_cast<BasePlugin*(*)()>(getFunction(handle, "CreatePlugin"));
		auto destroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(handle, "DestroyPlugin"));

		if (!createFunc || !destroyFunc) {
			std::cerr << "[PluginManager] Plugin missing required functions: " << dllToLoad << std::endl;
			unloadLibrary(handle);
			return false;
		}

		PluginInfo info;
		info.name = pluginName;
		info.path = pluginMainDir;
		info.activeDllPath = dllToLoad;
		info.stagingPath = pluginStagingDir;
		info.currentVersion = currentVersion;
		info.nextVersion = currentVersion + 1;
		info.handle = handle;
		info.loaded = true;
		info.enabled = false;
		info.hotReloadPending = false;
		info.createFunc = createFunc;
		info.destroyFunc = destroyFunc;
		info.lastScanTime = std::filesystem::file_time_type::clock::now();
		info.stagingWriteTime = std::filesystem::file_time_type{};

		plugins[pluginName] = info;
		std::cout << "[PluginManager] Plugin loaded successfully: " << pluginName
			<< " (version " << currentVersion << ")" << std::endl;
		return true;
	}

	bool PluginManager::checkStagingForUpdates(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		PluginInfo& plugin = it->second;

		if (!std::filesystem::exists(plugin.stagingPath)) {
			return false;
		}

		std::string stagingDllPath = findPluginDll(plugin.stagingPath, pluginName);

		if (stagingDllPath.empty()) {
			return false;
		}

		auto currentWriteTime = std::filesystem::last_write_time(stagingDllPath);

		if (currentWriteTime > plugin.stagingWriteTime) {
			std::cout << "[PluginManager] NEW DLL DETECTED in staging for: " << pluginName << std::endl;
			plugin.stagingWriteTime = currentWriteTime;
			return true;
		}

		return false;
	}

	bool PluginManager::createVersionedDllFromStaging(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		PluginInfo& plugin = it->second;
		std::string stagingDllPath = findPluginDll(plugin.stagingPath, pluginName);

		if (stagingDllPath.empty()) {
			std::cerr << "[PluginManager] No staging DLL found for: " << pluginName << std::endl;
			return false;
		}

		std::string versionedDllName = getVersionedDllName(pluginName, plugin.nextVersion);
		std::string newVersionedDllPath = plugin.path + "/" + versionedDllName;

		try {
			std::cout << "[PluginManager] Creating versioned DLL v" << plugin.nextVersion
				<< " from staging..." << std::endl;

			if (!copyFile(stagingDllPath, newVersionedDllPath)) {
				std::cerr << "[PluginManager] Failed to copy staging DLL to versioned location" << std::endl;
				return false;
			}

			std::filesystem::remove(stagingDllPath);

			plugin.currentVersion = plugin.nextVersion;
			plugin.nextVersion++;
			plugin.activeDllPath = newVersionedDllPath;

			std::cout << "[PluginManager] Successfully created versioned DLL v" << plugin.currentVersion
				<< ": " << newVersionedDllPath << std::endl;

			cleanupOldVersionedDlls(pluginName, 3);

			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Failed to create versioned DLL: " << e.what() << std::endl;
			return false;
		}
	}

	bool PluginManager::safeReloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		PluginInfo& plugin = it->second;
		bool wasEnabled = plugin.enabled;

		std::cout << "[PluginManager] === PERFORMING VERSIONED HOT RELOAD FOR: " << pluginName << " ===" << std::endl;
		std::cout << "[PluginManager] Current version: " << plugin.currentVersion
			<< ", target version: " << plugin.nextVersion << std::endl;

		if (wasEnabled) {
			std::cout << "[PluginManager] Step 1: Disabling plugin..." << std::endl;
			if (!disablePlugin(pluginName)) {
				std::cerr << "[PluginManager] Failed to disable plugin during reload" << std::endl;
				return false;
			}
		}

		std::cout << "[PluginManager] Step 2: Unloading old DLL..." << std::endl;
		if (plugin.handle) {
			unloadLibrary(plugin.handle);
			plugin.handle = nullptr;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::cout << "[PluginManager] Step 3: Creating new versioned DLL from staging..." << std::endl;
		if (!createVersionedDllFromStaging(pluginName)) {
			std::cerr << "[PluginManager] Failed to create new versioned DLL" << std::endl;
			return false;
		}

		cleanupOldVersionedDlls(pluginName, 2);

		std::cout << "[PluginManager] Step 4: Loading new versioned DLL..." << std::endl;
		plugin.handle = loadDynamicLibrary(plugin.activeDllPath);
		if (!plugin.handle) {
			std::cerr << "[PluginManager] Failed to load new versioned DLL: " << plugin.activeDllPath << std::endl;
			plugin.loaded = false;
			return false;
		}

		std::cout << "[PluginManager] Step 5: Getting function pointers..." << std::endl;
		plugin.createFunc = reinterpret_cast<BasePlugin*(*)()>(getFunction(plugin.handle, "CreatePlugin"));
		plugin.destroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(plugin.handle, "DestroyPlugin"));

		if (!plugin.createFunc || !plugin.destroyFunc) {
			std::cerr << "[PluginManager] Reloaded plugin missing required functions" << std::endl;
			unloadLibrary(plugin.handle);
			plugin.handle = nullptr;
			plugin.loaded = false;
			return false;
		}

		if (wasEnabled) {
			std::cout << "[PluginManager] Step 6: Re-enabling plugin..." << std::endl;
			if (!enablePlugin(pluginName)) {
				std::cerr << "[PluginManager] Failed to re-enable plugin after reload" << std::endl;
				return false;
			}
		}

		plugin.hotReloadPending = false;
		std::cout << "[PluginManager] === VERSIONED HOT RELOAD COMPLETED SUCCESSFULLY ===" << std::endl;
		std::cout << "[PluginManager] Active DLL is now: " << plugin.activeDllPath << std::endl;
		return true;
	}

	void PluginManager::checkForChanges() {
		if (!hotReloadEnabled && !hotReloadForced) return;

		for (auto& pair : plugins) {
			auto& plugin = pair.second;
			if (!plugin.loaded) continue;

			if (checkStagingForUpdates(pair.first)) {
				plugin.hotReloadPending = true;
				std::cout << "[PluginManager] MARKED FOR HOT RELOAD: " << pair.first
					<< " (will create version " << plugin.nextVersion << ")" << std::endl;
			}
		}
	}

	void PluginManager::processPendingReloads() {
		if (!hotReloadEnabled && !hotReloadForced) return;

		for (auto& pair : plugins) {
			auto& plugin = pair.second;
			if (plugin.hotReloadPending) {
				std::cout << "[PluginManager] PROCESSING HOT RELOAD: " << pair.first << std::endl;
				safeReloadPlugin(pair.first);
			}
		}
	}

	void PluginManager::updatePlugins(float deltaTime) {
		if (hotReloadEnabled || hotReloadForced) {
			timeSinceLastCheck += deltaTime;
			if (timeSinceLastCheck >= hotReloadCheckInterval) {
				checkForChanges();
				processPendingReloads();
				timeSinceLastCheck = 0.0f;
			}
		}

		for (auto& pair : plugins) {
			auto& plugin = pair.second;
			if (plugin.enabled && plugin.instance) {
				plugin.instance->OnUpdate(deltaTime);
			}
		}
	}

	bool PluginManager::copyFile(const std::string& source, const std::string& destination) {
		try {
			std::filesystem::create_directories(std::filesystem::path(destination).parent_path());
			std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Failed to copy file: " << e.what() << std::endl;
			return false;
		}
	}

	bool PluginManager::enablePlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end() || !it->second.loaded) {
			std::cerr << "[PluginManager] Plugin not found or not loaded: " << pluginName << std::endl;
			return false;
		}

		PluginInfo& plugin = it->second;
		if (plugin.enabled) {
			return true;
		}

		try {
			std::cout << "[PluginManager] Creating plugin instance for: " << pluginName << std::endl;
			plugin.instance = plugin.createFunc();
			if (!plugin.instance) {
				std::cerr << "[PluginManager] Failed to create plugin instance" << std::endl;
				return false;
			}

			plugin.version = plugin.instance->GetVersion();
			std::cout << "[PluginManager] Plugin instance created, version: " << plugin.version << std::endl;

			// ============================================
			// WITH SHARED DLLS: No need to pass registry pointers!
			// The plugin automatically uses the same registries from AniEngineCore.dll
			// ============================================

			std::cout << "[PluginManager] Calling OnEngineInit with direct EntityManager access" << std::endl;
			if (!plugin.instance->OnEngineInit(entityManager)) {
				std::cerr << "[PluginManager] Plugin initialization failed" << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			plugin.instance->SetInitialized(true);
			plugin.enabled = true;

			if (pluginState) {
				pluginState->SetPluginState(pluginName, true, true, plugin.path, plugin.currentVersion);
			}

			std::cout << "[PluginManager] Plugin enabled successfully: " << pluginName << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[PluginManager] Exception during plugin enable: " << e.what() << std::endl;
			if (plugin.instance) {
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
			}
			return false;
		}

		return true;
	}

	// ============================================
	// SIMPLIFIED: No component/system tracking
	// ============================================
	bool PluginManager::disablePlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end() || !it->second.enabled) {
			return false;
		}

		PluginInfo& plugin = it->second;

		std::cout << "[PluginManager] Disabling plugin: " << pluginName << std::endl;

		plugin.instance->OnShutdown();
		plugin.instance->SetInitialized(false);

		// SIMPLIFIED: Only cleanup views (if overridden by StudioPluginManager)
		// Components/systems are managed by EntityManager
		cleanupPluginViews(pluginName);

		plugin.destroyFunc(plugin.instance);
		plugin.instance = nullptr;
		plugin.enabled = false;

		if (pluginState) {
			pluginState->SetPluginState(pluginName, true, false, plugin.path, plugin.currentVersion);
		}

		std::cout << "[PluginManager] Plugin disabled: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::unloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		PluginInfo& plugin = it->second;
		if (plugin.enabled) {
			disablePlugin(pluginName);
		}

		if (plugin.handle) {
			unloadLibrary(plugin.handle);
		}

		if (pluginState) {
			pluginState->RemovePluginState(pluginName);
		}

		plugins.erase(it);
		std::cout << "[PluginManager] Plugin unloaded: " << pluginName << std::endl;
		return true;
	}

	bool PluginManager::reloadPlugin(const std::string& pluginName) {
		auto it = plugins.find(pluginName);
		if (it == plugins.end()) return false;

		bool wasEnabled = it->second.enabled;
		std::string path = it->second.path;

		if (!unloadPlugin(pluginName)) return false;
		if (!loadPlugin(path)) return false;
		if (wasEnabled && !enablePlugin(pluginName)) return false;

		return true;
	}

	void PluginManager::scanPluginDirectory(const std::string& directory) {
		if (!std::filesystem::exists(directory)) {
			return;
		}

		std::cout << "[PluginManager] Scanning plugin directory (discovery only): " << directory << std::endl;

		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.is_directory()) {
				std::string pluginName = entry.path().filename().string();
				if (pluginName == "staging") continue;

				std::cout << "[PluginManager] Found plugin directory: " << pluginName << std::endl;
			}
		}
	}

	std::vector<PluginInfo> PluginManager::getLoadedPlugins() const {
		std::vector<PluginInfo> loadedPlugins;
		for (const auto& pair : plugins) {
			loadedPlugins.push_back(pair.second);
		}
		return loadedPlugins;
	}

	BasePlugin* PluginManager::getPlugin(const std::string& name) const {
		auto it = plugins.find(name);
		return (it != plugins.end()) ? it->second.instance : nullptr;
	}

	void* PluginManager::loadDynamicLibrary(const std::string& path) {
#ifdef _WIN32
		return ::LoadLibraryA(path.c_str());
#else
		return dlopen(path.c_str(), RTLD_LAZY);
#endif
	}

	void PluginManager::unloadLibrary(void* handle) {
		if (!handle) return;
#ifdef _WIN32
		::FreeLibrary(static_cast<HMODULE>(handle));
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
#else
		dlclose(handle);
#endif
	}

	void* PluginManager::getFunction(void* handle, const std::string& name) {
		if (!handle) return nullptr;
#ifdef _WIN32
		return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), name.c_str()));
#else
		return dlsym(handle, name.c_str());
#endif
	}

} // namespace Plugins