// PluginManager.cpp
#include "PluginManager.hpp"
#include "EntityManager.hpp"
#include "PluginState.hpp"
#include "EngineContext.hpp"
#include "FilePathSystem.hpp"
#include "Log.hpp"

#include <filesystem>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <regex>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Plugins {

    PluginManager::PluginManager(ECS::EntityManager& entityMgr)
        : entityManager(entityMgr) {
        ANI_LOG_INFO("[PluginManager] Constructor - simplified manager created");
        stagingDirectory = "";
        InitializePluginStateManager();
        hotReloadEnabled = false;
        hotReloadForced = false;
        ANI_LOG_INFO("[PluginManager] Hot reload disabled by default");
    }

    PluginManager::~PluginManager() {
        ANI_LOG_INFO("[PluginManager] Destructor - cleaning up plugins...");
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
        pluginState->SetEntityManager(&entityManager);
        ANI_LOG_DEBUG("[PluginManager] Plugin state manager created");
    }

    void PluginManager::SetGlobalDataPath(const std::string& dataPath) {
        ANI_LOG_DEBUG("[PluginManager] SetGlobalDataPath called (ignored - using project state only)");
    }

    void PluginManager::LoadGlobalPluginState() {
        ANI_LOG_DEBUG("[PluginManager] LoadGlobalPluginState called (ignored - using project state only)");
    }

    void PluginManager::SaveGlobalPluginState() {
        ANI_LOG_DEBUG("[PluginManager] SaveGlobalPluginState called (ignored - using project state only)");
    }

    void PluginManager::UseGlobalPluginState() {
        ANI_LOG_DEBUG("[PluginManager] UseGlobalPluginState called (ignored - using project state only)");
    }

    void PluginManager::SetProjectContext(const std::string& projectPath) {
        if (!pluginState) {
            ANI_LOG_WARN("[PluginManager] SetProjectContext: pluginState not initialized");
            return;
        }
        ANI_LOG_INFO("[PluginManager] Setting project context: %s", projectPath.c_str());
        SaveCurrentPluginState();
        pluginState->SetCurrentProjectPath(projectPath);
        pluginState->LoadProjectPluginState();
        LoadPluginsFromState();
        ANI_LOG_INFO("[PluginManager] Project plugin context applied");
    }

    void PluginManager::SaveProjectPluginState() {
        if (!pluginState) {
            ANI_LOG_WARN("[PluginManager] SaveProjectPluginState: pluginState not initialized");
            return;
        }
        ANI_LOG_INFO("[PluginManager] Saving project plugin state...");
        SaveCurrentPluginState();
        pluginState->SaveProjectPluginState();
        ANI_LOG_INFO("[PluginManager] Project plugin state saved");
    }

    void PluginManager::LoadPluginsFromState() {
        if (!pluginState) {
            ANI_LOG_WARN("[PluginManager] LoadPluginsFromState: pluginState not initialized");
            return;
        }
        auto allPluginStates = pluginState->GetAllPluginStates();
        ANI_LOG_INFO("[PluginManager] Loading plugins from state (%zu plugins)", allPluginStates.size());

        for (const auto& [pluginName, state] : allPluginStates) {
            ANI_LOG_DEBUG("[PluginManager] Processing plugin from state: %s (loaded: %s, enabled: %s)",
                pluginName.c_str(),
                state.loaded ? "true" : "false",
                state.enabled ? "true" : "false");

            if (state.loaded) {
                if (plugins.find(pluginName) != plugins.end() && plugins[pluginName].loaded) {
                    ANI_LOG_TRACE("[PluginManager] Plugin already loaded: %s", pluginName.c_str());
                }
                else {
                    std::string pluginPath = state.path;
                    if (pluginPath.empty()) {
                        ANI_LOG_WARN("[PluginManager] No path in state for plugin: %s", pluginName.c_str());
                        continue;
                    }
                    ANI_LOG_INFO("[PluginManager] Loading plugin: %s from %s",
                        pluginName.c_str(), pluginPath.c_str());
                    if (loadPlugin(pluginPath)) {
                        ANI_LOG_INFO("[PluginManager] Successfully loaded plugin: %s", pluginName.c_str());
                    }
                    else {
                        ANI_LOG_ERROR("[PluginManager] Failed to load plugin: %s", pluginName.c_str());
                        continue;
                    }
                }

                if (state.enabled) {
                    if (plugins[pluginName].enabled) {
                        ANI_LOG_TRACE("[PluginManager] Plugin already enabled: %s", pluginName.c_str());
                    }
                    else {
                        ANI_LOG_INFO("[PluginManager] Enabling plugin: %s", pluginName.c_str());
                        if (!enablePlugin(pluginName)) {
                            ANI_LOG_ERROR("[PluginManager] Failed to enable plugin: %s", pluginName.c_str());
                        }
                    }
                }
            }
        }
        ANI_LOG_INFO("[PluginManager] Plugin loading from state complete");
    }

    void PluginManager::SaveCurrentPluginState() {
        if (!pluginState) {
            ANI_LOG_WARN("[PluginManager] SaveCurrentPluginState: pluginState not initialized");
            return;
        }
        ANI_LOG_DEBUG("[PluginManager] Saving current plugin state...");
        for (const auto& [pluginName, info] : plugins) {
            pluginState->SetPluginState(pluginName, info.loaded, info.enabled, info.path, info.currentVersion);
        }
        ANI_LOG_DEBUG("[PluginManager] Current plugin state saved to memory");
    }

    void PluginManager::setStagingDirectory(const std::string& basePluginsDir) {
        stagingDirectory = basePluginsDir;
        ANI_LOG_INFO("[PluginManager] Staging directory reference set to: %s", stagingDirectory.c_str());
    }

    void PluginManager::enableHotReload(bool enable) {
        hotReloadEnabled = enable;
        ANI_LOG_INFO("[PluginManager] Hot reload %s", enable ? "enabled" : "disabled");
    }

    void PluginManager::setHotReloadForce(bool force) {
        hotReloadForced = force;
        if (force) {
            hotReloadEnabled = true;
            ANI_LOG_INFO("[PluginManager] Hot reload FORCE ENABLED (for development)");
        }
        else {
            ANI_LOG_INFO("[PluginManager] Hot reload force disabled");
        }
    }

    void PluginManager::setupPluginDirectories(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            ANI_LOG_WARN("[PluginManager] Cannot setup directories - plugin not in registry: %s",
                pluginName.c_str());
            return;
        }

        std::string pluginMainDir = it->second.path;
        std::string pluginStagingDir = pluginMainDir + "/staging";

        if (!std::filesystem::exists(pluginStagingDir)) {
            std::filesystem::create_directories(pluginStagingDir);
            ANI_LOG_DEBUG("[PluginManager] Created staging directory: %s", pluginStagingDir.c_str());
        }

        ANI_LOG_DEBUG("[PluginManager] Setup directories for plugin: %s", pluginName.c_str());
        ANI_LOG_DEBUG("  Main: %s", pluginMainDir.c_str());
        ANI_LOG_DEBUG("  Staging: %s", pluginStagingDir.c_str());
    }

    bool PluginManager::moveFile(const std::string& source, const std::string& destination) {
        try {
            std::filesystem::rename(source, destination);
            ANI_LOG_DEBUG("[PluginManager] File moved: %s -> %s",
                source.c_str(), destination.c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[PluginManager] File move failed: %s", e.what());
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
                ANI_LOG_DEBUG("[PluginManager] Found plugin DLL: %s", fullPath.c_str());
                return fullPath;
            }
        }

        ANI_LOG_DEBUG("[PluginManager] No DLL found for plugin: %s in %s",
            pluginName.c_str(), pluginDir.c_str());
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
            ANI_LOG_DEBUG("[PluginManager] findNewestVersionedDll: directory does not exist: %s",
                pluginDir.c_str());
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
            ANI_LOG_DEBUG("[PluginManager] Found newest versioned DLL for %s: v%u at %s",
                pluginName.c_str(), highestVersion, newestDll.c_str());
        }
        else {
            ANI_LOG_DEBUG("[PluginManager] No versioned DLL found for: %s", pluginName.c_str());
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

        ANI_LOG_DEBUG("[PluginManager] extractVersionFromDllName: no version pattern in '%s'",
            filename.c_str());
        return 0;
    }

    void PluginManager::cleanupOldVersionedDlls(const std::string& pluginName, uint32_t keepVersionsCount) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            ANI_LOG_TRACE("[PluginManager] cleanupOldVersionedDlls: plugin not in registry: %s",
                pluginName.c_str());
            return;
        }

        std::string pluginMainDir = it->second.path;
        if (!std::filesystem::exists(pluginMainDir)) {
            ANI_LOG_TRACE("[PluginManager] cleanupOldVersionedDlls: dir does not exist: %s",
                pluginMainDir.c_str());
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
            ANI_LOG_DEBUG("[PluginManager] Cleaning up old DLL: v%u", versionedDlls[i].first);
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
                ANI_LOG_TRACE("[PluginManager] Staging DLL is too new (%llds), waiting for build to complete...",
                    static_cast<long long>(fileAge));
                return false;
            }

            if (currentWriteTime > info.stagingWriteTime) {
                info.stagingWriteTime = currentWriteTime;
                ANI_LOG_DEBUG("[PluginManager] Staging DLL updated for: %s", pluginName.c_str());
                ANI_LOG_DEBUG("[PluginManager] File age: %lld seconds", static_cast<long long>(fileAge));
                return true;
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[PluginManager] Error checking staging updates: %s", e.what());
        }

        return false;
    }

    bool PluginManager::createVersionedDllFromStaging(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            ANI_LOG_WARN("[PluginManager] createVersionedDllFromStaging: plugin not in registry: %s",
                pluginName.c_str());
            return false;
        }

        PluginInfo& info = it->second;
        if (info.stagingPath.empty()) {
            ANI_LOG_ERROR("[PluginManager] No staging path configured for: %s", pluginName.c_str());
            return false;
        }

        std::string stagingDllPath = info.stagingPath + "/" + pluginName;
#ifdef _WIN32
        stagingDllPath += ".dll";
#else
        stagingDllPath += ".so";
#endif

        if (!std::filesystem::exists(stagingDllPath)) {
            ANI_LOG_ERROR("[PluginManager] No staging DLL found for: %s", pluginName.c_str());
            return false;
        }

        std::string currentVersionDll = info.path + "/" + getVersionedDllName(pluginName, info.currentVersion);
        if (std::filesystem::exists(currentVersionDll)) {
            auto stagingSize = std::filesystem::file_size(stagingDllPath);
            auto currentSize = std::filesystem::file_size(currentVersionDll);
            auto stagingTime = std::filesystem::last_write_time(stagingDllPath);
            auto currentTime = std::filesystem::last_write_time(currentVersionDll);

            if (stagingSize == currentSize && stagingTime <= currentTime) {
                ANI_LOG_DEBUG("[PluginManager] Staging DLL is identical to current version, skipping");
                return false;
            }
        }

        std::string pluginMainDir = info.path;
        std::string versionedDllName = getVersionedDllName(pluginName, info.nextVersion);
        std::string newDllPath = pluginMainDir + "/" + versionedDllName;

        ANI_LOG_DEBUG("[PluginManager] Creating new versioned DLL v%u", info.nextVersion);
        ANI_LOG_DEBUG("  Source: %s", stagingDllPath.c_str());
        ANI_LOG_DEBUG("  Destination: %s", newDllPath.c_str());

        try {
            if (std::filesystem::exists(newDllPath)) {
                std::filesystem::remove(newDllPath);
            }

            std::filesystem::copy_file(stagingDllPath, newDllPath,
                std::filesystem::copy_options::overwrite_existing);

            if (!std::filesystem::exists(newDllPath)) {
                ANI_LOG_ERROR("[PluginManager] Destination file not created: %s", newDllPath.c_str());
                return false;
            }

            auto srcSize = std::filesystem::file_size(stagingDllPath);
            auto dstSize = std::filesystem::file_size(newDllPath);

            if (srcSize != dstSize) {
                ANI_LOG_ERROR("[PluginManager] File size mismatch after copy: src=%llu, dst=%llu",
                    static_cast<unsigned long long>(srcSize),
                    static_cast<unsigned long long>(dstSize));
                return false;
            }

            ANI_LOG_DEBUG("[PluginManager] Successfully created versioned DLL v%u (%llu bytes)",
                info.nextVersion, static_cast<unsigned long long>(dstSize));

            info.stagingWriteTime = std::filesystem::last_write_time(stagingDllPath);

            try {
                std::filesystem::remove(stagingDllPath);
                ANI_LOG_DEBUG("[PluginManager] Removed staging DLL: %s", stagingDllPath.c_str());
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[PluginManager] Failed to remove staging DLL: %s", e.what());
            }

            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("[PluginManager] Failed to copy staging DLL: %s", e.what());
            return false;
        }
    }

    bool PluginManager::safeReloadPlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            ANI_LOG_ERROR("[PluginManager] safeReloadPlugin: plugin not loaded: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& info = it->second;
        bool wasEnabled = info.enabled;

        ANI_LOG_INFO("[PluginManager] Starting safe reload for: %s", pluginName.c_str());
        ANI_LOG_DEBUG("[PluginManager] Current version: v%u", info.currentVersion);
        ANI_LOG_DEBUG("[PluginManager] Next version: v%u", info.nextVersion);

        if (wasEnabled) {
            ANI_LOG_DEBUG("[PluginManager] Disabling plugin for reload...");
            if (!disablePlugin(pluginName)) {
                ANI_LOG_ERROR("[PluginManager] Failed to disable plugin for reload");
                return false;
            }
        }

        if (info.destroyFunc && info.instance) {
            ANI_LOG_DEBUG("[PluginManager] Destroying plugin instance...");
            info.destroyFunc(info.instance);
            info.instance = nullptr;
        }

        if (info.handle) {
            ANI_LOG_DEBUG("[PluginManager] Unloading old DLL...");
            unloadLibrary(info.handle);
            info.handle = nullptr;
        }

        ANI_LOG_DEBUG("[PluginManager] Waiting for DLL to be released...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::string versionedDllName = getVersionedDllName(pluginName, info.nextVersion);
        std::string newDllPath = info.path + "/" + versionedDllName;

        if (!std::filesystem::exists(newDllPath)) {
            ANI_LOG_ERROR("[PluginManager] New DLL not found: %s", newDllPath.c_str());
            return false;
        }

        ANI_LOG_DEBUG("[PluginManager] Loading new DLL: %s", newDllPath.c_str());

        void* newHandle = loadDynamicLibrary(newDllPath);
        if (!newHandle) {
            ANI_LOG_ERROR("[PluginManager] Failed to load new DLL");
            return false;
        }

        auto newCreateFunc = reinterpret_cast<BasePlugin * (*)()>(getFunction(newHandle, "CreatePlugin"));
        auto newDestroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(newHandle, "DestroyPlugin"));

        if (!newCreateFunc || !newDestroyFunc) {
            ANI_LOG_ERROR("[PluginManager] Failed to load plugin functions from new DLL");
            unloadLibrary(newHandle);
            return false;
        }

        BasePlugin* newInstance = newCreateFunc();
        if (!newInstance) {
            ANI_LOG_ERROR("[PluginManager] Failed to create plugin instance from new DLL");
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

        ANI_LOG_INFO("[PluginManager] SUCCESS: Plugin reloaded: %s", pluginName.c_str());
        ANI_LOG_DEBUG("[PluginManager] New version: v%u", info.currentVersion);

        if (wasEnabled) {
            ANI_LOG_DEBUG("[PluginManager] Re-enabling plugin...");
            if (!enablePlugin(pluginName)) {
                ANI_LOG_WARN("[PluginManager] Warning: Plugin reloaded but failed to re-enable");
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
                ANI_LOG_INFO("[PluginManager] Update detected for: %s", pluginName.c_str());

                if (createVersionedDllFromStaging(pluginName)) {
                    info.hotReloadPending = true;
                    ANI_LOG_INFO("[PluginManager] Hot reload pending for: %s (will reload to v%u on next update cycle)",
                        pluginName.c_str(), info.nextVersion);
                }
                else {
                    ANI_LOG_ERROR("[PluginManager] Failed to prepare hot reload for: %s", pluginName.c_str());
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
            ANI_LOG_INFO("[PluginManager] Processing pending reload for: %s", pluginName.c_str());
            if (safeReloadPlugin(pluginName)) {
                ANI_LOG_INFO("[PluginManager] Successfully completed hot reload for: %s", pluginName.c_str());
            }
            else {
                ANI_LOG_ERROR("[PluginManager] Failed to complete hot reload for: %s", pluginName.c_str());
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

        ANI_LOG_INFO("[PluginManager] ======================================");
        ANI_LOG_INFO("[PluginManager] Loading plugin: %s", pluginName.c_str());
        ANI_LOG_INFO("[PluginManager] From path: %s", pluginPath.c_str());
        ANI_LOG_INFO("[PluginManager] ======================================");

        auto it = plugins.find(pluginName);
        if (it != plugins.end() && it->second.loaded) {
            ANI_LOG_TRACE("[PluginManager] Plugin already loaded: %s", pluginName.c_str());
            return true;
        }

        if (!std::filesystem::exists(pluginPath)) {
            ANI_LOG_ERROR("[PluginManager] Plugin path does not exist: %s", pluginPath.c_str());
            return false;
        }

        PluginInfo& info = plugins[pluginName];
        info.name = pluginName;
        info.path = pluginPath;

        ANI_LOG_DEBUG("[PluginManager] Plugin info initialized with path: %s", info.path.c_str());

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
                    ANI_LOG_DEBUG("[PluginManager] Staging directory found and tracked: %s",
                        pluginStagingDir.c_str());
                }
            }
            catch (const std::exception& e) {
                ANI_LOG_WARN("[PluginManager] Error setting up staging tracking: %s", e.what());
            }
        }

        std::string newestVersionedDll = findNewestVersionedDll(pluginPath, pluginName);
        uint32_t loadedVersion = 0;

        if (!newestVersionedDll.empty()) {
            loadedVersion = extractVersionFromDllName(newestVersionedDll, pluginName);
            ANI_LOG_DEBUG("[PluginManager] Found existing versioned DLL v%u", loadedVersion);
        }
        else {
            std::string stagingDll = pluginStagingDir + "/" + pluginName;
#ifdef _WIN32
            stagingDll += ".dll";
#else
            stagingDll += ".so";
#endif

            if (!std::filesystem::exists(stagingDll)) {
                ANI_LOG_ERROR("[PluginManager] No DLL found in plugin directory: %s", pluginPath.c_str());
                plugins.erase(pluginName);
                return false;
            }

            loadedVersion = 1;
            std::string versionedDllName = getVersionedDllName(pluginName, loadedVersion);
            std::string newDllPath = pluginPath + "/" + versionedDllName;

            ANI_LOG_DEBUG("[PluginManager] Creating initial versioned DLL v1...");
            ANI_LOG_DEBUG("  Source: %s", stagingDll.c_str());
            ANI_LOG_DEBUG("  Destination: %s", newDllPath.c_str());

            if (!copyFile(stagingDll, newDllPath)) {
                ANI_LOG_ERROR("[PluginManager] Failed to create initial versioned DLL");
                plugins.erase(pluginName);
                return false;
            }
            newestVersionedDll = newDllPath;
        }

        ANI_LOG_DEBUG("[PluginManager] Loading DLL: %s", newestVersionedDll.c_str());

        info.handle = loadDynamicLibrary(newestVersionedDll);
        if (!info.handle) {
            ANI_LOG_ERROR("[PluginManager] Failed to load plugin DLL: %s", newestVersionedDll.c_str());
            plugins.erase(pluginName);
            return false;
        }

        info.createFunc = reinterpret_cast<BasePlugin * (*)()>(getFunction(info.handle, "CreatePlugin"));
        info.destroyFunc = reinterpret_cast<void(*)(BasePlugin*)>(getFunction(info.handle, "DestroyPlugin"));

        if (!info.createFunc || !info.destroyFunc) {
            ANI_LOG_ERROR("[PluginManager] Failed to load plugin functions from DLL");
            unloadLibrary(info.handle);
            plugins.erase(pluginName);
            return false;
        }

        info.instance = info.createFunc();
        if (!info.instance) {
            ANI_LOG_ERROR("[PluginManager] Failed to create plugin instance");
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

        ANI_LOG_INFO("[PluginManager] ======================================");
        ANI_LOG_INFO("[PluginManager] SUCCESS: Plugin loaded: %s", pluginName.c_str());
        ANI_LOG_INFO("[PluginManager] Version: %u (DLL v%u)", info.version, info.currentVersion);
        ANI_LOG_INFO("[PluginManager] Path: %s", info.path.c_str());
        ANI_LOG_INFO("[PluginManager] ======================================");

        return true;
    }

    bool PluginManager::enablePlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            ANI_LOG_WARN("[PluginManager] enablePlugin: plugin not loaded: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& info = it->second;

        if (info.enabled) {
            ANI_LOG_TRACE("[PluginManager] Plugin already enabled: %s", pluginName.c_str());
            return true;
        }

        ANI_LOG_INFO("[PluginManager] Enabling plugin: %s", pluginName.c_str());

        if (!info.instance) {
            ANI_LOG_ERROR("[PluginManager] No plugin instance to enable");
            return false;
        }

        if (!engineContext.expired()) {
            auto ctx = engineContext.lock();
            if (ctx) {
                info.instance->SetEngineContext(ctx);
            }
        }

        if (!info.instance->OnEngineInit(entityManager)) {
            ANI_LOG_ERROR("[PluginManager] Plugin OnEngineInit() returned false: %s", pluginName.c_str());
            return false;
        }

        info.instance->SetInitialized(true);
        info.enabled = true;

        OnPluginEnabled(pluginName);

        ANI_LOG_INFO("[PluginManager] Plugin enabled: %s", pluginName.c_str());

        return true;
    }

    bool PluginManager::disablePlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            ANI_LOG_WARN("[PluginManager] disablePlugin: plugin not loaded: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& info = it->second;

        if (!info.enabled) {
            ANI_LOG_TRACE("[PluginManager] Plugin already disabled: %s", pluginName.c_str());
            return true;
        }

        ANI_LOG_INFO("[PluginManager] Disabling plugin: %s", pluginName.c_str());

        if (info.instance) {
            info.instance->OnShutdown();
            info.instance->SetInitialized(false);
        }

        info.enabled = false;

        OnPluginDisabled(pluginName);

        ANI_LOG_INFO("[PluginManager] Plugin disabled: %s", pluginName.c_str());

        return true;
    }

    bool PluginManager::unloadPlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            ANI_LOG_WARN("[PluginManager] unloadPlugin: plugin not found: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& info = it->second;

        if (info.enabled) {
            ANI_LOG_DEBUG("[PluginManager] Force-disabling plugin before unload...");
            if (info.instance) {
                info.instance->OnShutdown();
                info.instance->SetInitialized(false);
            }
            info.enabled = false;
        }

        if (info.destroyFunc && info.instance) {
            ANI_LOG_DEBUG("[PluginManager] Destroying plugin instance...");
            info.destroyFunc(info.instance);
            info.instance = nullptr;
        }

        if (info.handle) {
            ANI_LOG_DEBUG("[PluginManager] Unloading plugin DLL...");
            unloadLibrary(info.handle);
            info.handle = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (pluginState) {
            pluginState->RemovePluginState(pluginName);
        }

        plugins.erase(pluginName);

        ANI_LOG_INFO("[PluginManager] Plugin unloaded: %s", pluginName.c_str());
        return true;
    }

    bool PluginManager::reloadPlugin(const std::string& pluginName) {
        auto it = plugins.find(pluginName);
        if (it == plugins.end()) {
            ANI_LOG_WARN("[PluginManager] reloadPlugin: plugin not found: %s", pluginName.c_str());
            return false;
        }

        bool wasEnabled = it->second.enabled;
        std::string path = it->second.path;

        ANI_LOG_INFO("[PluginManager] Reloading plugin: %s", pluginName.c_str());

        if (!unloadPlugin(pluginName)) return false;
        if (!loadPlugin(path)) return false;
        if (wasEnabled && !enablePlugin(pluginName)) return false;

        ANI_LOG_INFO("[PluginManager] Plugin reload complete: %s", pluginName.c_str());
        return true;
    }

    void PluginManager::scanPluginDirectory(const std::string& directory) {
        if (!std::filesystem::exists(directory)) {
            ANI_LOG_DEBUG("[PluginManager] scanPluginDirectory: directory does not exist: %s",
                directory.c_str());
            return;
        }

        ANI_LOG_DEBUG("[PluginManager] Scanning plugin directory (discovery only): %s", directory.c_str());

        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_directory()) {
                std::string pluginName = entry.path().filename().string();
                if (pluginName == "staging") continue;

                ANI_LOG_DEBUG("[PluginManager] Found plugin directory: %s", pluginName.c_str());
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
            ANI_LOG_DEBUG("[PluginManager] File copied: %s -> %s",
                source.c_str(), destination.c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_WARN("[PluginManager] File copy failed: %s", e.what());
            return false;
        }
    }

} // namespace Plugins