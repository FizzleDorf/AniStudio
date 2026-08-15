// StudioPluginManager.cpp
#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
#include "FilePathSystem.hpp"
#include "FilePathComponent.hpp"
#include "ProjectSystem.hpp"
#include <imgui.h>
#include <iostream>
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
        std::cout << "[StudioPluginManager] Constructor - studio manager created with ImGui context: "
            << mainImGuiContext << std::endl;

        auto fs = entityMgr.GetSystem<ECS::FilePathSystem>();
        if (fs) {
            m_pluginDirectory = fs->GetPath("Plugins");
            if (m_pluginDirectory.empty()) {
                m_pluginDirectory = (std::filesystem::current_path() / "plugins").string();
                fs->SetPath("Plugins", m_pluginDirectory);
            }
            std::cout << "[StudioPluginManager] Plugin directory: " << m_pluginDirectory << std::endl;
        }
        else {
            m_pluginDirectory = (std::filesystem::current_path() / "plugins").string();
        }

        setStagingDirectory(m_pluginDirectory);
    }

    bool StudioPluginManager::enablePlugin(const std::string& pluginName) {
        std::cout << "[StudioPluginManager] Enabling plugin with studio support: "
            << pluginName << std::endl;

        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            std::cerr << "[StudioPluginManager] Plugin not found or not loaded: "
                << pluginName << std::endl;
            return false;
        }

        PluginInfo& plugin = it->second;
        if (plugin.enabled) {
            std::cout << "[StudioPluginManager] Plugin already enabled: "
                << pluginName << std::endl;
            return true;
        }

        try {
            std::shared_ptr<ANI::EngineContext> engineContextPtr;
            if (!engineContext.expired()) {
                engineContextPtr = engineContext.lock();
            }

            if (!engineContextPtr && studioContext) {
                engineContextPtr = std::static_pointer_cast<ANI::EngineContext>(studioContext);
                std::cout << "[StudioPluginManager] Using StudioContext as EngineContext for plugin" << std::endl;
            }

            if (engineContextPtr) {
                plugin.instance->SetEngineContext(engineContextPtr);
                std::cout << "[StudioPluginManager] EngineContext set for plugin: " << pluginName << std::endl;
            }
            else {
                std::cerr << "[StudioPluginManager] WARNING: No EngineContext available for plugin!" << std::endl;
            }

            if (studioContext) {
                plugin.instance->SetStudioContext(studioContext);
            }

            if (mainImGuiContext) {
                std::cout << "[StudioPluginManager] Setting ImGui context for plugin: "
                    << mainImGuiContext << std::endl;
                plugin.instance->SetImGuiContext(mainImGuiContext);
            }

            std::cout << "[StudioPluginManager] Calling OnEngineInit..." << std::endl;
            if (!plugin.instance->OnEngineInit(entityManager)) {
                std::cerr << "[StudioPluginManager] Plugin engine initialization failed: "
                    << pluginName << std::endl;
                return false;
            }

            std::cout << "[StudioPluginManager] Calling OnStudioInit..." << std::endl;
            if (!plugin.instance->OnStudioInit(entityManager, viewManager)) {
                std::cerr << "[StudioPluginManager] Plugin studio initialization failed: "
                    << pluginName << std::endl;
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
                            std::cout << "[StudioPluginManager] Added view " << viewName
                                << " to workspace " << wsID << std::endl;
                        }
                        catch (const std::exception& e) {
                            std::cerr << "[StudioPluginManager] Failed to add view " << viewName
                                << " to workspace " << wsID << ": " << e.what() << std::endl;
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
                std::cout << "[StudioPluginManager] Reloaded viewstate after plugin re-enable." << std::endl;
            }

            std::cout << "[StudioPluginManager] Plugin enabled with studio support: "
                << pluginName << std::endl;

            std::cout << "[StudioPluginManager] === POST-ENABLE DEBUG ===" << std::endl;
            entityManager.DebugPrintRegisteredComponents();
            std::cout << "[StudioPluginManager] =======================\n" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioPluginManager] Exception during plugin enable: "
                << e.what() << std::endl;
            return false;
        }

        return true;
    }

    bool StudioPluginManager::disablePlugin(const std::string& pluginName) {
        std::cout << "[StudioPluginManager] Disabling plugin: " << pluginName << std::endl;

        auto it = plugins.find(pluginName);
        if (it == plugins.end() || !it->second.loaded) {
            std::cerr << "[StudioPluginManager] Plugin not loaded: " << pluginName << std::endl;
            return false;
        }

        PluginInfo& plugin = it->second;
        if (!plugin.enabled) {
            std::cout << "[StudioPluginManager] Plugin already disabled: " << pluginName << std::endl;
            return true;
        }

        try {
            SaveViewState();
            m_viewStateSaved = true;
            std::cout << "[StudioPluginManager] Saved viewstate before plugin shutdown." << std::endl;

            if (plugin.instance) {
                plugin.instance->OnShutdown();
                plugin.instance->SetInitialized(false);
            }

            auto viewIt = pluginViewNames.find(pluginName);
            if (viewIt != pluginViewNames.end()) {
                for (const std::string& viewName : viewIt->second) {
                    try {
                        viewManager.CloseAllViewsOfType(viewName);
                        std::cout << "[StudioPluginManager] Closed views of type: " << viewName << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[StudioPluginManager] Failed to close views of type " << viewName << ": " << e.what() << std::endl;
                    }
                }
                pluginViewNames.erase(viewIt);
            }

            plugin.enabled = false;

            std::cout << "[StudioPluginManager] Plugin disabled: " << pluginName << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioPluginManager] Exception during disable: " << e.what() << std::endl;
            return false;
        }
    }

    void StudioPluginManager::OnPluginEnabled(const std::string& pluginName) {
        std::cout << "[StudioPluginManager] Plugin enabled callback: " << pluginName << std::endl;
    }

    void StudioPluginManager::OnPluginDisabled(const std::string& pluginName) {
        std::cout << "[StudioPluginManager] Plugin disabled callback: " << pluginName << std::endl;
    }

    void StudioPluginManager::SetProjectContext(const std::string& projectPath) {
        PluginManager::SetProjectContext(projectPath);
        m_viewStateSaved = false;
        std::cout << "[StudioPluginManager] Loaded project plugin state from: " << projectPath << std::endl;
    }

    void StudioPluginManager::SetPluginDirectory(const std::string& dir) {
        m_pluginDirectory = dir;
        auto fs = entityManager.GetSystem<ECS::FilePathSystem>();
        if (fs) {
            fs->SetPath("Plugins", dir);
        }
        setStagingDirectory(dir);
        std::cout << "[StudioPluginManager] Plugin directory set: " << dir << std::endl;
    }

    void StudioPluginManager::SaveViewState() {
        auto projSys = entityManager.GetSystem<ANI::ProjectSystem>();
        if (projSys && projSys->IsProjectOpen()) {
            projSys->SaveViewState();
            std::cout << "[StudioPluginManager] Saved viewstate before plugin disable." << std::endl;
        }
    }

    void StudioPluginManager::LoadViewState() {
        auto projSys = entityManager.GetSystem<ANI::ProjectSystem>();
        if (projSys && projSys->IsProjectOpen()) {
            projSys->LoadViewState();
            std::cout << "[StudioPluginManager] Reloaded viewstate after plugin enable." << std::endl;
        }
    }

    void StudioPluginManager::LoadStagingPlugins(bool overrideExisting) {
        if (m_pluginDirectory.empty()) {
            std::cerr << "[StudioPluginManager] Plugin directory not set, cannot load staging plugins." << std::endl;
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

                std::cout << "[StudioPluginManager] Checking staging for plugin: " << pluginName
                    << " at " << pluginStagingDir << std::endl;

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
                        std::cout << "[StudioPluginManager] No DLL found in staging for: " << pluginName << std::endl;
                        continue;
                    }
                }

                std::cout << "[StudioPluginManager] Found staging plugin: " << pluginName << " at " << pluginDllPath << std::endl;

                auto it = plugins.find(pluginName);
                if (it != plugins.end() && it->second.loaded) {
                    std::cout << "[StudioPluginManager] Plugin already loaded, unloading first: " << pluginName << std::endl;
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

                std::cout << "[StudioPluginManager] Creating versioned DLL v" << newVersion << ": " << destDllPath << std::endl;

                try {
                    std::filesystem::rename(pluginDllPath, destDllPath);
                    std::cout << "[StudioPluginManager] Successfully moved staging DLL to versioned file: " << destDllPath << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "[StudioPluginManager] Failed to move DLL: " << e.what() << std::endl;
                    continue;
                }

                for (const auto& file : std::filesystem::directory_iterator(pluginStagingDir)) {
                    if (file.is_regular_file() && file.path().extension() != ".dll" &&
                        file.path().extension() != ".so" && file.path().extension() != ".dylib") {
                        std::string destFile = (pluginEntry.path() / file.path().filename()).string();
                        try {
                            std::filesystem::copy_file(file.path(), destFile);
                            std::cout << "[StudioPluginManager] Copied extra file: " << file.path().filename() << std::endl;
                        }
                        catch (...) {}
                    }
                }

                try {
                    if (std::filesystem::is_empty(pluginStagingDir)) {
                        std::filesystem::remove(pluginStagingDir);
                        std::cout << "[StudioPluginManager] Removed empty staging directory: " << pluginStagingDir << std::endl;
                    }
                }
                catch (...) {}

                if (!loadPlugin(pluginEntry.path().string())) {
                    std::cerr << "[StudioPluginManager] Failed to load plugin from: " << pluginEntry.path() << std::endl;
                }
                else {
                    std::cout << "[StudioPluginManager] Successfully loaded plugin " << pluginName
                        << " version v" << newVersion << std::endl;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "[StudioPluginManager] Exception loading staging plugins: " << e.what() << std::endl;
        }
    }

} // namespace Plugins