#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include "StudioContext.hpp"
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
			// Pass engine context if available
			if (!engineContext.expired()) {
				auto ctx = engineContext.lock();
				if (ctx) {
					plugin.instance->SetEngineContext(ctx);
				}
			}

			// Pass studio context if available
			if (studioContext) {
				plugin.instance->SetStudioContext(studioContext);
			}

			// Pass ImGui context if available
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

	void StudioPluginManager::cleanupPluginViews(const std::string& pluginName) {
		try {
			auto nameIt = pluginViewNames.find(pluginName);
			if (nameIt == pluginViewNames.end()) {
				std::cout << "[StudioPluginManager] No views to cleanup for plugin: " << pluginName << std::endl;
				return;
			}

			std::cout << "[StudioPluginManager] Cleaning up " << nameIt->second.size()
				<< " views for plugin: " << pluginName << std::endl;

			// First, remove from tracking
			std::vector<std::string> viewsToClean = std::move(nameIt->second);
			pluginViewNames.erase(nameIt);

			// Close and unregister views
			for (const std::string& viewName : viewsToClean) {
				std::cout << "[StudioPluginManager] Cleaning up view: " << viewName << std::endl;

				// Close all instances first
				viewManager.CloseAllViewsOfType(viewName);

				// Give time for views to close
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

				// Unregister the view type
				try {
					viewManager.UnregisterView(viewName);
					std::cout << "[StudioPluginManager] Unregistered view: " << viewName << std::endl;
				}
				catch (const std::exception& e) {
					std::cerr << "[StudioPluginManager] Failed to unregister view "
						<< viewName << ": " << e.what() << std::endl;
				}
			}

			std::cout << "[StudioPluginManager] View cleanup completed for plugin: " << pluginName << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioPluginManager] Error during view cleanup: " << e.what() << std::endl;
		}
	}

} // namespace Plugins