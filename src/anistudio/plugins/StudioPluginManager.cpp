// StudioPluginManager.cpp
#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
#include "FilePathSystem.hpp"
#include "FilePathComponent.hpp"
#include "ProjectSystem.hpp"
#include "Log.hpp"

#include <imgui.h>
#include <thread>
#include <filesystem>
#include <regex>
#include <fstream>

namespace Plugins {

    StudioPluginManager::StudioPluginManager(
        ECS::EntityManager& entityMgr,
        GUI::ViewManager& viewMgr,
        ImGuiContext* mainContext
    ) : PluginManager(entityMgr), viewManager(viewMgr), mainImGuiContext(mainContext), m_viewStateSaved(false) {
        ANI_LOG_INFO("StudioPluginManager constructor (ImGuiContext=%p)",
            static_cast<void*>(mainImGuiContext));

        auto fs = entityMgr.GetSystem<ECS::FilePathSystem>();
        if (fs) {
            m_pluginDirectory = fs->GetPath("Plugins");
            if (m_pluginDirectory.empty()) {
                m_pluginDirectory = (std::filesystem::current_path() / "plugins").string();
                fs->SetPath("Plugins", m_pluginDirectory);
                ANI_LOG_DEBUG("Plugin directory defaulted to: %s", m_pluginDirectory.c_str());
            }
            else {
                ANI_LOG_DEBUG("Plugin directory from FilePathSystem: %s", m_pluginDirectory.c_str());
            }
        }
        else {
            m_pluginDirectory = (std::filesystem::current_path() / "plugins").string();
            ANI_LOG_WARN("FilePathSystem unavailable, plugin directory defaulted to: %s",
                m_pluginDirectory.c_str());
        }

        setStagingDirectory(m_pluginDirectory);
    }

    bool StudioPluginManager::enablePlugin(const std::string& pluginName) {
        ANI_LOG_INFO("Enabling plugin with studio support: %s", pluginName.c_str());

        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            ANI_LOG_WARN("Plugin not found or not loaded: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& plugin = it->second;
        if (plugin.enabled) {
            ANI_LOG_TRACE("Plugin already enabled: %s", pluginName.c_str());
            return true;
        }

        try {
            std::shared_ptr<ANI::EngineContext> engineContextPtr;
            if (!engineContext.expired()) {
                engineContextPtr = engineContext.lock();
            }

            if (!engineContextPtr && studioContext) {
                engineContextPtr = std::static_pointer_cast<ANI::EngineContext>(studioContext);
                ANI_LOG_DEBUG("Using StudioContext as EngineContext for plugin: %s",
                    pluginName.c_str());
            }

            if (engineContextPtr) {
                plugin.instance->SetEngineContext(engineContextPtr);
                ANI_LOG_TRACE("EngineContext set for plugin: %s", pluginName.c_str());
            }
            else {
                ANI_LOG_WARN("No EngineContext available for plugin: %s", pluginName.c_str());
            }

            if (studioContext) {
                plugin.instance->SetStudioContext(studioContext);
            }

            if (mainImGuiContext) {
                ANI_LOG_TRACE("Setting ImGui context (%p) for plugin: %s",
                    static_cast<void*>(mainImGuiContext), pluginName.c_str());
                plugin.instance->SetImGuiContext(mainImGuiContext);
            }

            ANI_LOG_DEBUG("Calling OnEngineInit for plugin: %s", pluginName.c_str());
            if (!plugin.instance->OnEngineInit(entityManager)) {
                ANI_LOG_ERROR("Plugin engine initialization failed: %s", pluginName.c_str());
                return false;
            }

            ANI_LOG_DEBUG("Calling OnStudioInit for plugin: %s", pluginName.c_str());
            if (!plugin.instance->OnStudioInit(entityManager, viewManager)) {
                ANI_LOG_ERROR("Plugin studio initialization failed: %s", pluginName.c_str());
                return false;
            }

            auto viewNames = viewManager.GetViewsBySource(pluginName);
            if (!viewNames.empty()) {
                pluginViewNames[pluginName] = viewNames;
                auto allWorkspaces = viewManager.GetAllWorkspaces();
                for (const std::string& viewName : viewNames) {
                    GUI::ViewTypeID viewTypeID = viewManager.GetViewType(viewName);
                    for (GUI::WorkspaceID wsID : allWorkspaces) {
                        try {
                            viewManager.AddViewByType(wsID, viewTypeID);
                            ANI_LOG_DEBUG("Added view '%s' to workspace %zu",
                                viewName.c_str(), (size_t)wsID);
                        }
                        catch (const std::exception& e) {
                            ANI_LOG_WARN("Failed to add view '%s' to workspace %zu: %s",
                                viewName.c_str(), (size_t)wsID, e.what());
                        }
                    }
                }
            }

            plugin.instance->SetInitialized(true);
            plugin.enabled = true;

            if (pluginState) {
                pluginState->SetPluginState(pluginName, true, true, plugin.path, plugin.currentVersion);
            }

            if (m_viewStateSaved) {
                LoadViewState();
                m_viewStateSaved = false;
                ANI_LOG_DEBUG("Reloaded viewstate after plugin re-enable");
            }

            ANI_LOG_INFO("Plugin enabled with studio support: %s", pluginName.c_str());

            ANI_LOG_TRACE("Post-enable component dump for plugin: %s", pluginName.c_str());
            entityManager.DebugPrintRegisteredComponents();
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception during plugin enable '%s': %s",
                pluginName.c_str(), e.what());
            return false;
        }

        return true;
    }

    bool StudioPluginManager::disablePlugin(const std::string& pluginName) {
        ANI_LOG_INFO("Disabling plugin: %s", pluginName.c_str());

        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            ANI_LOG_WARN("Plugin not loaded: %s", pluginName.c_str());
            return false;
        }

        PluginInfo& plugin = it->second;
        if (!plugin.enabled) {
            ANI_LOG_TRACE("Plugin already disabled: %s", pluginName.c_str());
            return true;
        }

        try {
            SaveViewState();
            m_viewStateSaved = true;
            ANI_LOG_DEBUG("Saved viewstate before plugin shutdown");

            if (plugin.instance) {
                plugin.instance->OnShutdown();
                plugin.instance->SetInitialized(false);
            }

            auto viewIt = pluginViewNames.find(pluginName);
            if (viewIt != pluginViewNames.end()) {
                for (const std::string& viewName : viewIt->second) {
                    try {
                        viewManager.CloseAllViewsOfType(viewName);
                        ANI_LOG_DEBUG("Closed views of type: %s", viewName.c_str());
                    }
                    catch (const std::exception& e) {
                        ANI_LOG_WARN("Failed to close views of type '%s': %s",
                            viewName.c_str(), e.what());
                    }
                }
                pluginViewNames.erase(viewIt);
            }

            plugin.enabled = false;

            ANI_LOG_INFO("Plugin disabled: %s", pluginName.c_str());
            return true;
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception during disable '%s': %s",
                pluginName.c_str(), e.what());
            return false;
        }
    }

    void StudioPluginManager::OnPluginEnabled(const std::string& pluginName) {
        ANI_LOG_DEBUG("Plugin enabled callback: %s", pluginName.c_str());
    }

    void StudioPluginManager::OnPluginDisabled(const std::string& pluginName) {
        ANI_LOG_DEBUG("Plugin disabled callback: %s", pluginName.c_str());
    }

    void StudioPluginManager::SetProjectContext(const std::string& projectPath) {
        PluginManager::SetProjectContext(projectPath);
        m_viewStateSaved = false;
        ANI_LOG_INFO("Loaded project plugin state from: %s", projectPath.c_str());
    }

    void StudioPluginManager::SetPluginDirectory(const std::string& dir) {
        m_pluginDirectory = dir;
        auto fs = entityManager.GetSystem<ECS::FilePathSystem>();
        if (fs) {
            fs->SetPath("Plugins", dir);
        }
        setStagingDirectory(dir);
        ANI_LOG_INFO("Plugin directory set: %s", dir.c_str());
    }

    void StudioPluginManager::SaveViewState() {
        auto projSys = entityManager.GetSystem<ECS::ProjectSystem>();
        if (projSys && projSys->IsProjectOpen()) {
            projSys->SaveViewState();
            ANI_LOG_DEBUG("Saved viewstate before plugin disable");
        }
    }

    void StudioPluginManager::LoadViewState() {
        auto projSys = entityManager.GetSystem<ECS::ProjectSystem>();
        if (projSys && projSys->IsProjectOpen()) {
            projSys->LoadViewState();
            ANI_LOG_DEBUG("Reloaded viewstate after plugin enable");
        }
    }

    void StudioPluginManager::LoadStagingPlugins(bool overrideExisting) {
        if (m_pluginDirectory.empty()) {
            ANI_LOG_ERROR("Plugin directory not set, cannot load staging plugins");
            return;
        }

        try {
            for (const auto& pluginEntry : std::filesystem::directory_iterator(m_pluginDirectory)) {
                if (!pluginEntry.is_directory()) continue;

                std::string pluginName = pluginEntry.path().filename().string();
                if (pluginName == "staging") continue;

                std::filesystem::path pluginStagingDir = pluginEntry.path() / "staging";
                if (!std::filesystem::exists(pluginStagingDir)) {
                    continue;
                }

                ANI_LOG_DEBUG("Checking staging for plugin '%s' at %s",
                    pluginName.c_str(), pluginStagingDir.string().c_str());

                std::string pluginDllPath = (pluginStagingDir / (pluginName + ".dll")).string();
                if (!std::filesystem::exists(pluginDllPath)) {
                    bool found = false;
                    for (const auto& ext : { ".so", ".dylib" }) {
                        std::string altPath = (pluginStagingDir / (pluginName + ext)).string();
                        if (std::filesystem::exists(altPath)) {
                            pluginDllPath = altPath;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        ANI_LOG_TRACE("No DLL found in staging for: %s", pluginName.c_str());
                        continue;
                    }
                }

                ANI_LOG_DEBUG("Found staging plugin '%s' at %s",
                    pluginName.c_str(), pluginDllPath.c_str());

                auto it = plugins.find(pluginName);
                if (it != plugins.end() && it->second.loaded) {
                    ANI_LOG_DEBUG("Plugin already loaded, unloading first: %s", pluginName.c_str());
                    unloadPlugin(pluginName);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }

                int highestVersion = 0;
                std::regex versionPattern(pluginName + "\\.v(\\d+)\\.(dll|so|dylib)$");
                for (const auto& file : std::filesystem::directory_iterator(pluginEntry.path())) {
                    if (file.is_regular_file()) {
                        std::string filename = file.path().filename().string();
                        std::smatch matches;
                        if (std::regex_match(filename, matches, versionPattern)) {
                            int ver = std::stoi(matches[1].str());
                            if (ver > highestVersion) highestVersion = ver;
                        }
                    }
                }

                int newVersion = highestVersion + 1;
                std::string versionedDllName = pluginName + ".v" + std::to_string(newVersion) + ".dll";
                std::string destDllPath = (pluginEntry.path() / versionedDllName).string();

                ANI_LOG_DEBUG("Creating versioned DLL v%d: %s",
                    newVersion, destDllPath.c_str());

                try {
                    std::filesystem::rename(pluginDllPath, destDllPath);
                    ANI_LOG_DEBUG("Moved staging DLL to versioned file: %s", destDllPath.c_str());
                }
                catch (const std::exception& e) {
                    ANI_LOG_ERROR("Failed to move DLL for plugin '%s': %s",
                        pluginName.c_str(), e.what());
                    continue;
                }

                for (const auto& file : std::filesystem::directory_iterator(pluginStagingDir)) {
                    if (file.is_regular_file()) {
                        std::string destFile = (pluginEntry.path() / file.path().filename()).string();
                        try {
                            std::filesystem::copy_file(file.path(), destFile,
                                std::filesystem::copy_options::overwrite_existing);
                            ANI_LOG_TRACE("Copied staging file: %s",
                                file.path().filename().string().c_str());
                        }
                        catch (const std::exception& e) {
                            ANI_LOG_WARN("Failed to copy staging file '%s': %s",
                                file.path().filename().string().c_str(), e.what());
                        }
                    }
                }

                for (const auto& file : std::filesystem::directory_iterator(pluginStagingDir)) {
                    if (file.is_regular_file()) {
                        std::error_code ec;
                        std::filesystem::remove(file.path(), ec);
                        if (ec) {
                            ANI_LOG_WARN("Failed to delete staging file '%s': %s",
                                file.path().filename().string().c_str(), ec.message().c_str());
                        }
                        else {
                            ANI_LOG_TRACE("Deleted staging file: %s",
                                file.path().filename().string().c_str());
                        }
                    }
                }

                if (!loadPlugin(pluginEntry.path().string())) {
                    ANI_LOG_ERROR("Failed to load plugin from: %s",
                        pluginEntry.path().string().c_str());
                }
                else {
                    ANI_LOG_INFO("Successfully loaded plugin '%s' version v%d",
                        pluginName.c_str(), newVersion);
                }
            }
        }
        catch (const std::exception& e) {
            ANI_LOG_ERROR("Exception loading staging plugins: %s", e.what());
        }
    }

    void StudioPluginManager::PrepareProjectPlugins() {
        LoadStagingPlugins(true);

        if (!pluginState) {
            ANI_LOG_DEBUG("No plugin state, nothing to enable");
            return;
        }

        auto states = pluginState->GetAllPluginStates();
        for (const auto& [pluginName, state] : states) {
            if (!state.enabled) continue;

            auto it = plugins.find(pluginName);
            if (it == plugins.end() || !it->second.loaded) {
                ANI_LOG_WARN("Plugin '%s' is enabled in project state but not loaded; skipping enable",
                    pluginName.c_str());
                continue;
            }

            if (it->second.enabled) {
                continue;
            }

            ANI_LOG_INFO("Enabling project plugin: %s", pluginName.c_str());
            if (!enablePlugin(pluginName)) {
                ANI_LOG_ERROR("Failed to enable project plugin: %s", pluginName.c_str());
            }
        }
    }

} // namespace Plugins