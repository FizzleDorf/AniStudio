#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include <imgui.h>
#include <iostream>

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
			std::cout << "[StudioPluginManager] Creating plugin instance..." << std::endl;
			plugin.instance = plugin.createFunc();

			if (!plugin.instance) {
				std::cerr << "[StudioPluginManager] Failed to create plugin instance: "
					<< pluginName << std::endl;
				return false;
			}

			// ============================================
			// ADDED: PASS IMGUI CONTEXT TO PLUGIN
			// ============================================
			if (mainImGuiContext) {
				std::cout << "[StudioPluginManager] Setting ImGui context for plugin: "
					<< mainImGuiContext << std::endl;
				plugin.instance->SetImGuiContext(mainImGuiContext);
			}

			plugin.version = plugin.instance->GetVersion();
			std::cout << "[StudioPluginManager] Plugin instance created, version: "
				<< plugin.version << std::endl;

			std::cout << "[StudioPluginManager] Calling OnEngineInit..." << std::endl;
			if (!plugin.instance->OnEngineInit(entityManager)) {
				std::cerr << "[StudioPluginManager] Plugin engine initialization failed: "
					<< pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			std::cout << "[StudioPluginManager] Calling OnStudioInit..." << std::endl;
			if (!plugin.instance->OnStudioInit(entityManager, viewManager)) {
				std::cerr << "[StudioPluginManager] Plugin studio initialization failed: "
					<< pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
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
			if (plugin.instance) {
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
			}
			return false;
		}

		return true;
	}

	void StudioPluginManager::cleanupPluginViews(const std::string& pluginName) {
		auto nameIt = pluginViewNames.find(pluginName);
		if (nameIt == pluginViewNames.end()) {
			std::cout << "[StudioPluginManager] No views to cleanup for plugin: "
				<< pluginName << std::endl;
			return;
		}

		std::cout << "[StudioPluginManager] Cleaning up " << nameIt->second.size()
			<< " views for plugin: " << pluginName << std::endl;

		for (const std::string& viewName : nameIt->second) {
			std::cout << "[StudioPluginManager] Unregistering view: " << viewName << std::endl;

			// Close all instances of this view type
			viewManager.CloseAllViewsOfType(viewName);

			// Unregister from ViewManager - this handles everything
			viewManager.UnregisterViewType(viewName);
		}

		// Unregister the plugin as a view source
		viewManager.UnregisterViewSource("plugin");

		pluginViewNames.erase(nameIt);

		std::cout << "[StudioPluginManager] View cleanup completed for plugin: "
			<< pluginName << std::endl;
	}

} // namespace Plugins