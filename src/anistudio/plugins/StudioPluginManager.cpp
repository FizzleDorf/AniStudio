#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
#include <imgui.h>
#include <iostream>
#include <thread>

namespace Plugins {

    StudioPluginManager::StudioPluginManager(
        ECS::EntityManager& entityMgr,
        GUI::ViewManager& viewMgr,
        ImGuiContext* mainContext
    ) : PluginManager(entityMgr), viewManager(viewMgr), mainImGuiContext(mainContext) {
        std::cout << "[StudioPluginManager] Constructor - studio manager created with ImGui context: "
            << mainImGuiContext << std::endl;
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

} // namespace Plugins