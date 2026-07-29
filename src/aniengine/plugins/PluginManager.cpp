// PluginManager.cpp (already contains the staging removal, unchanged)
#include "PluginManager.hpp"
#include "EntityManager.hpp"
#include "PluginState.hpp"
#include "EngineContext.hpp"
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
        stagingDirectory = "";
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
        std::cout << "[PluginManager] Switching to global plugin state..." << std::endl;
        SaveCurrentPluginState();
        pluginState->UseGlobalState();
        LoadPluginsFromState();
        std::cout << "[PluginManager] Switched to global plugin state" << std::endl;
    }

    void PluginManager::LoadPluginsFromState() {
        if (!pluginState) return;
        auto allPluginStates = pluginState->GetAllPluginStates();
        std::cout << "[PluginManager] Loading plugins from state (" << allPluginStates.size() << " plugins)" << std::endl;

        for (const auto& [pluginName, state] : allPluginStates) {
            std::cout << "[PluginManager] Processing plugin from state: " << pluginName
                << " (loaded: " << state.loaded << ", enabled: " << state.enabled << ")" << std::endl;

            if (state.loaded) {
                if (plugins.find(pluginName) != plugins.end() && plugins[pluginName].loaded) {
                    std::cout << "[PluginManager] Plugin already loaded: " << pluginName << std::endl;
                }
                else {
                    std::string pluginPath = state.path;
                    if (pluginPath.empty()) {
                        std::cerr << "[PluginManager] No path in state for plugin: " << pluginName << std::endl;
                        continue;
                    }
                    std::cout << "[PluginManager] Loading plugin: " << pluginName << " from " << pluginPath << std::endl;
                    if (loadPlugin(pluginPath)) {
                        std::cout << "[PluginManager] Successfully loaded plugin: " << pluginName << std::endl;
                    }
                    else {
                        std::cerr << "[PluginManager] Failed to load plugin: " << pluginName << std::endl;
                        continue;
                    }
                }

                if (state.enabled) {
                    if (plugins[pluginName].enabled) {
                        std::cout << "[PluginManager] Plugin already enabled: " << pluginName << std::endl;
                    }
                    else {
                        std::cout << "[PluginManager] Enabling plugin: " << pluginName << std::endl;
                        if (!enablePlugin(pluginName)) {
                            std::cerr << "[PluginManager] Failed to enable plugin: " << pluginName << std::endl;
                        }
                    }
                }
            }
        }
        std::cout << "[PluginManager] Plugin loading from state complete" << std::endl;
    }

    void PluginManager::SaveCurrentPluginState() {
        if (!pluginState) return;
        std::cout << "[PluginManager] Saving current plugin state..." << std::endl;
        for (const auto& [pluginName, info] : plugins) {
            pluginState->SetPluginState(pluginName, info.loaded, info.enabled, info.path, info.currentVersion);
        }
        std::cout << "[PluginManager] Current plugin state saved to memory" << std::endl;
    }

    void PluginManager::setStagingDirectory(const std::string& basePluginsDir) {
        stagingDirectory = basePluginsDir;
        std::cout << "[PluginManager] Staging directory reference set to: " << stagingDirectory << std::endl;
    }

    void PluginManager::enableHotReload(bool enable) {
        hotReloadEnabled = enable;
        std::cout << "[PluginManager] Hot reload " << (enable ? "enabled" : "disabled") << std::endl;
    }

    void PluginManager::setHotReloadForce(bool force) {
        hotReloadForced = force;
        if (force) {
            hotReloadEnabled = true;
            std::cout << "[PluginManager] Hot reload FORCE ENABLED (for development)" << std::endl;
        }
        else {
            std::cout << "[PluginManager] Hot reload force disabled" << std::endl;
        }
    }

    void PluginManager::setupPluginDirectories(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            std::cerr << "[PluginManager] Cannot setup directories - plugin not in registry: " << pluginName << std::endl;
            return;
        }

        std::string pluginMainDir = it->second.path;
        std::string pluginStagingDir = pluginMainDir + "/staging";

        if (!std::filesystem::exists(pluginStagingDir)) {
            std::filesystem::create_directories(pluginStagingDir);
            std::cout << "[PluginManager] Created staging directory: " << pluginStagingDir << std::endl;
        }

        std::cout << "[PluginManager] Setup directories for plugin: " << pluginName << std::endl;
        std::cout << "  Main: " << pluginMainDir << std::endl;
        std::cout << "  Staging: " << pluginStagingDir << std::endl;
    }

    bool PluginManager::moveFile(const std::string& source, const std::string& destination) {
        try {
            std::filesystem::rename(source, destination);
            std::cout << "[PluginManager] File moved successfully from " << source << " to " << destination << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginManager] File move failed: " << e.what() << std::endl;
            return false;
        }
    }

    std::string PluginManager::findPluginDll(const std::string& pluginDir, const std::string& pluginName) {
        std::vector<std::string> possibleNames = {
            pluginName + ".dll",
            pluginName + ".so",
            "lib" + pluginName + ".so"
        };

        for (const auto& dllName : possibleNames) {
            std::string fullPath = pluginDir + "/" + dllName;
            if (std::filesystem::exists(fullPath)) {
                std::cout << "[PluginManager] Found plugin DLL: " << fullPath << std::endl;
                return fullPath;
            }
        }

        std::cout << "[PluginManager] No DLL found for plugin: " << pluginName << " in " << pluginDir << std::endl;
        return "";
    }

    std::string PluginManager::getVersionedDllName(const std::string& pluginName, uint32_t version) {
#ifdef _WIN32
        return pluginName + ".v" + std::to_string(version) + ".dll";
#else
        return pluginName + ".v" + std::to_string(version) + ".so";
#endif
    }

    std::string PluginManager::findNewestVersionedDll(const std::string& pluginDir, const std::string& pluginName) {
        uint32_t highestVersion = 0;
        std::string newestDll;

        if (!std::filesystem::exists(pluginDir)) {
            return "";
        }

        std::regex versionPattern(pluginName + "\\.v(\\d+)\\.(dll|so)");

        for (const auto& entry : std::filesystem::directory_iterator(pluginDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::smatch matches;

                if (std::regex_match(filename, matches, versionPattern)) {
                    uint32_t version = std::stoul(matches[1].str());
                    if (version > highestVersion) {
                        highestVersion = version;
                        newestDll = entry.path().string();
                    }
                }
            }
        }

        if (!newestDll.empty()) {
            std::cout << "[PluginManager] Found newest versioned DLL for " << pluginName
                << ": v" << highestVersion << " at " << newestDll << std::endl;
        }

        return newestDll;
    }

    uint32_t PluginManager::extractVersionFromDllName(const std::string& dllPath, const std::string& pluginName) {
        std::filesystem::path path(dllPath);
        std::string filename = path.filename().string();
        std::regex versionPattern(pluginName + "\\.v(\\d+)\\.(dll|so)");
        std::smatch matches;

        if (std::regex_match(filename, matches, versionPattern)) {
            return std::stoul(matches[1].str());
        }

        return 0;
    }

    void PluginManager::cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            return;
        }

        std::string pluginMainDir = it->second.path;
        if (!std::filesystem::exists(pluginMainDir)) {
            return;
        }

        std::vector<std::pair<uint32_t, std::string>> versionedDlls;
        std::regex versionPattern(pluginName + "\\.v(\\d+)\\.(dll|so)");

        for (const auto& entry : std::filesystem::directory_iterator(pluginMainDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::smatch matches;

                if (std::regex_match(filename, matches, versionPattern)) {
                    uint32_t version = std::stoul(matches[1].str());
                    versionedDlls.push_back({ version, entry.path().string() });
                }
            }
        }

        if (versionedDlls.size() <= keepVersionsCount) {
            return;
        }

        std::sort(versionedDlls.begin(), versionedDlls.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });

        for (size_t i = keepVersionsCount; i < versionedDlls.size(); ++i) {
            std::cout << "[PluginManager] Cleaning up old DLL: v" << versionedDlls[i].first << std::endl;
            std::filesystem::remove(versionedDlls[i].second);
        }
    }

    bool PluginManager::checkStagingForUpdates(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            return false;
        }

        PluginInfo& info = it->second;
        if (info.stagingPath.empty()) {
            return false;
        }

        std::string stagingDllPath = info.stagingPath + "/" + pluginName;
#ifdef _WIN32
        stagingDllPath += ".dll";
#else
        stagingDllPath += ".so";
#endif

        if (!std::filesystem::exists(stagingDllPath)) {
            return false;
        }

        try {
            auto currentWriteTime = std::filesystem::last_write_time(stagingDllPath);
            auto now = std::filesystem::file_time_type::clock::now();
            auto fileAge = std::chrono::duration_cast<std::chrono::seconds>(now - currentWriteTime).count();

            if (fileAge < 2) {
                std::cout << "[PluginManager] Staging DLL is too new (" << fileAge << "s), waiting for build to complete..." << std::endl;
                return false;
            }

            if (currentWriteTime > info.stagingWriteTime) {
                info.stagingWriteTime = currentWriteTime;
                std::cout << "[PluginManager] Staging DLL updated for: " << pluginName << std::endl;
                std::cout << "[PluginManager] File age: " << fileAge << " seconds" << std::endl;
                return true;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginManager] Error checking staging updates: " << e.what() << std::endl;
        }

        return false;
    }

    bool PluginManager::createVersionedDllFromStaging(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            return false;
        }

        PluginInfo& info = it->second;
        if (info.stagingPath.empty()) {
            std::cerr << "[PluginManager] No staging path configured for: " << pluginName << std::endl;
            return false;
        }

        std::string stagingDllPath = info.stagingPath + "/" + pluginName;
#ifdef _WIN32
        stagingDllPath += ".dll";
#else
        stagingDllPath += ".so";
#endif

        if (!std::filesystem::exists(stagingDllPath)) {
            std::cerr << "[PluginManager] No staging DLL found for: " << pluginName << std::endl;
            return false;
        }

        std::string currentVersionDll = info.path + "/" + getVersionedDllName(pluginName, info.currentVersion);
        if (std::filesystem::exists(currentVersionDll)) {
            auto stagingSize = std::filesystem::file_size(stagingDllPath);
            auto currentSize = std::filesystem::file_size(currentVersionDll);
            auto stagingTime = std::filesystem::last_write_time(stagingDllPath);
            auto currentTime = std::filesystem::last_write_time(currentVersionDll);

            if (stagingSize == currentSize && stagingTime <= currentTime) {
                std::cout << "[PluginManager] Staging DLL is identical to current version, skipping" << std::endl;
                return false;
            }
        }

        std::string pluginMainDir = info.path;
        std::string versionedDllName = getVersionedDllName(pluginName, info.nextVersion);
        std::string newDllPath = pluginMainDir + "/" + versionedDllName;

        std::cout << "[PluginManager] Creating new versioned DLL v" << info.nextVersion << std::endl;
        std::cout << "  Source: " << stagingDllPath << std::endl;
        std::cout << "  Destination: " << newDllPath << std::endl;

        try {
            if (std::filesystem::exists(newDllPath)) {
                std::filesystem::remove(newDllPath);
            }

            std::filesystem::copy_file(stagingDllPath, newDllPath,
                std::filesystem::copy_options::overwrite_existing);

            if (!std::filesystem::exists(newDllPath)) {
                std::cerr << "[PluginManager] Destination file not created: " << newDllPath << std::endl;
                return false;
            }

            auto srcSize = std::filesystem::file_size(stagingDllPath);
            auto dstSize = std::filesystem::file_size(newDllPath);

            if (srcSize != dstSize) {
                std::cerr << "[PluginManager] File size mismatch after copy: src="
                    << srcSize << ", dst=" << dstSize << std::endl;
                return false;
            }

            std::cout << "[PluginManager] Successfully created versioned DLL v"
                << info.nextVersion << " (" << dstSize << " bytes)" << std::endl;

            info.stagingWriteTime = std::filesystem::last_write_time(stagingDllPath);

            try {
                std::filesystem::remove(stagingDllPath);
                std::cout << "[PluginManager] Removed staging DLL: " << stagingDllPath << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "[PluginManager] Failed to remove staging DLL: " << e.what() << std::endl;
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginManager] Failed to copy staging DLL: " << e.what() << std::endl;
            return false;
        }
    }

    bool PluginManager::safeReloadPlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            std::cerr << "[PluginManager] Plugin not loaded: " << pluginName << std::endl;
            return false;
        }

        PluginInfo& info = it->second;
        bool wasEnabled = info.enabled;

        std::cout << "[PluginManager] Starting safe reload for: " << pluginName << std::endl;
        std::cout << "[PluginManager] Current version: v" << info.currentVersion << std::endl;
        std::cout << "[PluginManager] Next version: v" << info.nextVersion << std::endl;

        if (wasEnabled) {
            std::cout << "[PluginManager] Disabling plugin for reload..." << std::endl;
            if (!disablePlugin(pluginName)) {
                std::cerr << "[PluginManager] Failed to disable plugin for reload" << std::endl;
                return false;
            }
        }

        if (info.destroyFunc && info.instance) {
            std::cout << "[PluginManager] Destroying plugin instance..." << std::endl;
            info.destroyFunc(info.instance);
            info.instance = nullptr;
        }

        if (info.handle) {
            std::cout << "[PluginManager] Unloading old DLL..." << std::endl;
            unloadLibrary(info.handle);
            info.handle = nullptr;
        }

        std::cout << "[PluginManager] Waiting for DLL to be released..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string versionedDllName = getVersionedDllName(pluginName, info.nextVersion);
        std::string newDllPath = info.path + "/" + versionedDllName;

        if (!std::filesystem::exists(newDllPath)) {
            std::cerr << "[PluginManager] New DLL not found: " << newDllPath << std::endl;
            return false;
        }

        std::cout << "[PluginManager] Loading new DLL: " << newDllPath << std::endl;

        void* newHandle = loadDynamicLibrary(newDllPath);
        if (!newHandle) {
            std::cerr << "[PluginManager] Failed to load new DLL" << std::endl;
            return false;
        }

        auto newCreateFunc = reinterpret_cast<BasePlugin * (*)()>(getFunction(newHandle, "CreatePlugin"));
        auto newDestroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(newHandle, "DestroyPlugin"));

        if (!newCreateFunc || !newDestroyFunc) {
            std::cerr << "[PluginManager] Failed to load plugin functions from new DLL" << std::endl;
            unloadLibrary(newHandle);
            return false;
        }

        BasePlugin* newInstance = newCreateFunc();
        if (!newInstance) {
            std::cerr << "[PluginManager] Failed to create plugin instance from new DLL" << std::endl;
            unloadLibrary(newHandle);
            return false;
        }

        info.handle = newHandle;
        info.createFunc = newCreateFunc;
        info.destroyFunc = newDestroyFunc;
        info.instance = newInstance;
        info.activeDllPath = newDllPath;
        info.currentVersion = info.nextVersion;
        info.nextVersion++;
        info.loaded = true;
        info.version = newInstance->GetVersion();

        std::cout << "[PluginManager] SUCCESS: Plugin reloaded: " << pluginName << std::endl;
        std::cout << "[PluginManager] New version: v" << info.currentVersion << std::endl;

        if (wasEnabled) {
            std::cout << "[PluginManager] Re-enabling plugin..." << std::endl;
            if (!enablePlugin(pluginName)) {
                std::cerr << "[PluginManager] Warning: Plugin reloaded but failed to re-enable" << std::endl;
            }
        }

        info.hotReloadPending = false;

        return true;
    }

    void PluginManager::checkForChanges() {
        if (!hotReloadEnabled && !hotReloadForced) return;

        for (auto& pair : plugins) {
            const std::string& pluginName = pair.first;
            PluginInfo& info = pair.second;

            if (!info.loaded) continue;

            bool hasUpdate = checkStagingForUpdates(pluginName);

            if (hasUpdate) {
                std::cout << "[PluginManager] Update detected for: " << pluginName << std::endl;

                if (createVersionedDllFromStaging(pluginName)) {
                    info.hotReloadPending = true;
                    std::cout << "[PluginManager] Hot reload pending for: " << pluginName
                        << " (will reload to v" << info.nextVersion << " on next update cycle)" << std::endl;
                }
                else {
                    std::cerr << "[PluginManager] Failed to prepare hot reload for: " << pluginName << std::endl;
                }
            }
        }
    }

    void PluginManager::processPendingReloads() {
        std::vector<std::string> pluginsToReload;

        for (const auto& pair : plugins) {
            if (pair.second.hotReloadPending && pair.second.loaded) {
                pluginsToReload.push_back(pair.first);
            }
        }

        for (const std::string& pluginName : pluginsToReload) {
            std::cout << "[PluginManager] Processing pending reload for: " << pluginName << std::endl;
            if (safeReloadPlugin(pluginName)) {
                std::cout << "[PluginManager] Successfully completed hot reload for: " << pluginName << std::endl;
            }
            else {
                std::cerr << "[PluginManager] Failed to complete hot reload for: " << pluginName << std::endl;
                plugins[pluginName].hotReloadPending = false;
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
            if (pair.second.loaded && pair.second.enabled && pair.second.instance) {
                pair.second.instance->OnUpdate(deltaTime);
            }
        }
    }

    bool PluginManager::loadPlugin(const std::string& pluginPath) {
        std::filesystem::path path(pluginPath);
        std::string pluginName = path.filename().string();

        std::cout << "[PluginManager] ======================================" << std::endl;
        std::cout << "[PluginManager] Loading plugin: " << pluginName << std::endl;
        std::cout << "[PluginManager] From path: " << pluginPath << std::endl;
        std::cout << "[PluginManager] ======================================" << std::endl;

        auto it = plugins.find(pluginName);
        if (it != plugins.end() && it->second.loaded) {
            std::cout << "[PluginManager] Plugin already loaded: " << pluginName << std::endl;
            return true;
        }

        if (!std::filesystem::exists(pluginPath)) {
            std::cerr << "[PluginManager] ERROR: Plugin path does not exist: " << pluginPath << std::endl;
            return false;
        }

        PluginInfo& info = plugins[pluginName];
        info.name = pluginName;
        info.path = pluginPath;

        std::cout << "[PluginManager] Plugin info initialized with path: " << info.path << std::endl;

        setupPluginDirectories(pluginName);

        std::string pluginStagingDir = pluginPath + "/staging";
        info.stagingPath = pluginStagingDir;

        if (std::filesystem::exists(pluginStagingDir)) {
            try {
                std::string stagingDll = pluginStagingDir + "/" + pluginName;
#ifdef _WIN32
                stagingDll += ".dll";
#else
                stagingDll += ".so";
#endif
                if (std::filesystem::exists(stagingDll)) {
                    info.stagingWriteTime = std::filesystem::last_write_time(stagingDll);
                    std::cout << "[PluginManager] Staging directory found and tracked: " << pluginStagingDir << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[PluginManager] Error setting up staging tracking: " << e.what() << std::endl;
            }
        }

        std::string newestVersionedDll = findNewestVersionedDll(pluginPath, pluginName);
        uint32_t loadedVersion = 0;

        if (!newestVersionedDll.empty()) {
            loadedVersion = extractVersionFromDllName(newestVersionedDll, pluginName);
            std::cout << "[PluginManager] Found existing versioned DLL v" << loadedVersion << std::endl;
        }
        else {
            std::string stagingDll = pluginStagingDir + "/" + pluginName;
#ifdef _WIN32
            stagingDll += ".dll";
#else
            stagingDll += ".so";
#endif

            if (!std::filesystem::exists(stagingDll)) {
                std::cerr << "[PluginManager] ERROR: No DLL found in plugin directory: " << pluginPath << std::endl;
                plugins.erase(pluginName);
                return false;
            }

            loadedVersion = 1;
            std::string versionedDllName = getVersionedDllName(pluginName, loadedVersion);
            std::string newDllPath = pluginPath + "/" + versionedDllName;

            std::cout << "[PluginManager] Creating initial versioned DLL v1..." << std::endl;
            std::cout << "  Source: " << stagingDll << std::endl;
            std::cout << "  Destination: " << newDllPath << std::endl;

            if (!copyFile(stagingDll, newDllPath)) {
                std::cerr << "[PluginManager] ERROR: Failed to create initial versioned DLL" << std::endl;
                plugins.erase(pluginName);
                return false;
            }
            newestVersionedDll = newDllPath;
        }

        std::cout << "[PluginManager] Loading DLL: " << newestVersionedDll << std::endl;

        info.handle = loadDynamicLibrary(newestVersionedDll);
        if (!info.handle) {
            std::cerr << "[PluginManager] ERROR: Failed to load plugin DLL" << std::endl;
            plugins.erase(pluginName);
            return false;
        }

        info.createFunc = reinterpret_cast<BasePlugin * (*)()>(getFunction(info.handle, "CreatePlugin"));
        info.destroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(info.handle, "DestroyPlugin"));

        if (!info.createFunc || !info.destroyFunc) {
            std::cerr << "[PluginManager] ERROR: Failed to load plugin functions" << std::endl;
            unloadLibrary(info.handle);
            plugins.erase(pluginName);
            return false;
        }

        info.instance = info.createFunc();
        if (!info.instance) {
            std::cerr << "[PluginManager] ERROR: Failed to create plugin instance" << std::endl;
            unloadLibrary(info.handle);
            plugins.erase(pluginName);
            return false;
        }

        if (!engineContext.expired()) {
            auto ctx = engineContext.lock();
            if (ctx) {
                info.instance->SetEngineContext(ctx);
            }
        }

        info.version = info.instance->GetVersion();
        info.activeDllPath = newestVersionedDll;
        info.currentVersion = loadedVersion;
        info.nextVersion = loadedVersion + 1;
        info.loaded = true;
        info.enabled = false;

        std::cout << "[PluginManager] ======================================" << std::endl;
        std::cout << "[PluginManager] SUCCESS: Plugin loaded: " << pluginName << std::endl;
        std::cout << "[PluginManager] Version: " << info.version << " (DLL v" << info.currentVersion << ")" << std::endl;
        std::cout << "[PluginManager] Path: " << info.path << std::endl;
        std::cout << "[PluginManager] ======================================" << std::endl;

        return true;
    }

    bool PluginManager::enablePlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            std::cerr << "[PluginManager] Plugin not loaded: " << pluginName << std::endl;
            return false;
        }

        PluginInfo& info = it->second;

        if (info.enabled) {
            std::cout << "[PluginManager] Plugin already enabled: " << pluginName << std::endl;
            return true;
        }

        std::cout << "[PluginManager] Enabling plugin: " << pluginName << std::endl;

        if (!info.instance) {
            std::cerr << "[PluginManager] No plugin instance to enable" << std::endl;
            return false;
        }

        if (!engineContext.expired()) {
            auto ctx = engineContext.lock();
            if (ctx) {
                info.instance->SetEngineContext(ctx);
            }
        }

        if (!info.instance->OnEngineInit(entityManager)) {
            std::cerr << "[PluginManager] Plugin OnEngineInit() returned false" << std::endl;
            return false;
        }

        info.instance->SetInitialized(true);
        info.enabled = true;
        std::cout << "[PluginManager] Plugin enabled: " << pluginName << std::endl;

        return true;
    }

    bool PluginManager::disablePlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            std::cerr << "[PluginManager] Plugin not loaded: " << pluginName << std::endl;
            return false;
        }

        PluginInfo& info = it->second;

        if (!info.enabled) {
            std::cout << "[PluginManager] Plugin already disabled: " << pluginName << std::endl;
            return true;
        }

        std::cout << "[PluginManager] Disabling plugin: " << pluginName << std::endl;

        if (info.instance) {
            std::cout << "[PluginManager] Calling OnShutdown for plugin cleanup..." << std::endl;
            info.instance->OnShutdown();
            info.instance->SetInitialized(false);
        }

        info.enabled = false;
        std::cout << "[PluginManager] Plugin disabled: " << pluginName << std::endl;

        return true;
    }

    bool PluginManager::unloadPlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            std::cerr << "[PluginManager] Plugin not found: " << pluginName << std::endl;
            return false;
        }

        PluginInfo& info = it->second;

        if (info.enabled) {
            std::cout << "[PluginManager] Force-disabling plugin before unload..." << std::endl;
            if (info.instance) {
                info.instance->OnShutdown();
                info.instance->SetInitialized(false);
            }
            info.enabled = false;
        }

        if (info.destroyFunc && info.instance) {
            std::cout << "[PluginManager] Destroying plugin instance..." << std::endl;
            info.destroyFunc(info.instance);
            info.instance = nullptr;
        }

        if (info.handle) {
            std::cout << "[PluginManager] Unloading plugin DLL..." << std::endl;
            unloadLibrary(info.handle);
            info.handle = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (pluginState) {
            pluginState->RemovePluginState(pluginName);
        }

        plugins.erase(pluginName);

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

    bool PluginManager::copyFile(const std::string& source, const std::string& destination) {
        try {
            std::filesystem::copy_file(source, destination,
                std::filesystem::copy_options::overwrite_existing);
            std::cout << "[PluginManager] File copied successfully" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[PluginManager] File copy failed: " << e.what() << std::endl;
            return false;
        }
    }

} // namespace Plugins