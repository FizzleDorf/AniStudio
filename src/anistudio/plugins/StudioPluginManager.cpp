#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include <imgui.h>
#include <iostream>

namespace Plugins {

	// StudioPluginRegistry implementation - NOW PROPERLY HANDLES IMGUI CONTEXT
	StudioPluginRegistry::StudioPluginRegistry(const std::string& pluginName,
		StudioPluginManager* manager,
		GUI::ViewManager& viewMgr,
		ImGuiContext* mainContext)
		: PluginRegistry(pluginName, manager, mainContext), viewManager(viewMgr), studioManager(manager), mainImGuiContext(mainContext) {
		std::cout << "[StudioPluginRegistry] Constructor - plugin: " << pluginName
			<< ", main ImGui context: " << mainImGuiContext << std::endl;
	}

	GUI::ViewTypeID StudioPluginRegistry::RegisterView(const ViewDescriptor& desc) {
		std::cout << "[StudioPluginRegistry] Registering view: " << desc.name
			<< " for plugin: " << GetCurrentPluginName()
			<< " with context: " << mainImGuiContext << std::endl;
		return studioManager->registerView(GetCurrentPluginName(), desc);
	}

	// StudioPluginManager implementation - NOW PROPERLY HANDLES IMGUI CONTEXT
	StudioPluginManager::StudioPluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr, ImGuiContext* mainContext)
		: PluginManager(entityMgr, mainContext), viewManager(viewMgr), mainImGuiContext(mainContext) {
		std::cout << "[StudioPluginManager] Constructor - studio manager created with ImGui context: " << mainImGuiContext << std::endl;
	}

	bool StudioPluginManager::enablePlugin(const std::string& pluginName) {
		std::cout << "[StudioPluginManager] Enabling plugin with studio support: " << pluginName << std::endl;

		auto it = plugins.find(pluginName);
		if (it == plugins.end() || !it->second.loaded) {
			std::cerr << "[StudioPluginManager] Plugin not found or not loaded: " << pluginName << std::endl;
			return false;
		}

		PluginInfo& plugin = it->second;
		if (plugin.enabled) {
			std::cout << "[StudioPluginManager] Plugin already enabled: " << pluginName << std::endl;
			return true;
		}

		try {
			// Create the plugin instance
			std::cout << "[StudioPluginManager] Creating plugin instance..." << std::endl;
			plugin.instance = plugin.createFunc();

			if (!plugin.instance) {
				std::cerr << "[StudioPluginManager] Failed to create plugin instance: " << pluginName << std::endl;
				return false;
			}

			// Get version from plugin instance
			plugin.version = plugin.instance->GetVersion();

			// Create a studio registry for this plugin - NOW WITH PROPER IMGUI CONTEXT
			StudioPluginRegistry registry(pluginName, this, viewManager, mainImGuiContext);

			// Call OnEngineInit first
			std::cout << "[StudioPluginManager] Calling OnEngineInit with registry that has ImGui context: "
				<< mainImGuiContext << std::endl;

			if (!plugin.instance->OnEngineInit(entityManager, registry)) {
				std::cerr << "[StudioPluginManager] Plugin engine initialization failed: " << pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			// Then call OnStudioInit - ALSO WITH PROPER IMGUI CONTEXT
			std::cout << "[StudioPluginManager] Calling OnStudioInit with registry that has ImGui context: "
				<< mainImGuiContext << std::endl;

			if (!plugin.instance->OnStudioInit(entityManager, viewManager, registry)) {
				std::cerr << "[StudioPluginManager] Plugin studio initialization failed: " << pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			plugin.instance->SetInitialized(true);
			plugin.enabled = true;

			std::cout << "[StudioPluginManager] Plugin enabled with studio support: " << pluginName << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "[StudioPluginManager] Exception during plugin enable: " << e.what() << std::endl;
			if (plugin.instance) {
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
			}
			return false;
		}

		return true;
	}

	GUI::ViewTypeID StudioPluginManager::registerView(const std::string& pluginName, const ViewDescriptor& desc) {
		std::cout << "[StudioPluginManager] Registering view: " << desc.name
			<< " for plugin: " << pluginName
			<< " with main context: " << mainImGuiContext << std::endl;

		GUI::ViewTypeID viewTypeID = GUI::ViewTypeRegistry::RegisterTypeByName(desc.name);

		if (viewTypeID == GUI::MAX_VIEW_COUNT) {
			std::cerr << "[StudioPluginManager] Failed to register view type: " << desc.name << std::endl;
			return 0;
		}

		// CRITICAL FIX: Capture the ImGui context by value and ensure it's valid
		ImGuiContext* contextToUse = mainImGuiContext;
		if (!contextToUse) {
			contextToUse = ImGui::GetCurrentContext();
			std::cerr << "[StudioPluginManager] WARNING: mainImGuiContext was null, using current: " << contextToUse << std::endl;
		}

		std::cout << "[StudioPluginManager] Will use ImGui context: " << contextToUse << std::endl;

		// CRITICAL FIX: Create a wrapper factory that passes the ImGui context to the plugin
		viewManager.RegisterViewWithFactory(
			desc.name,
			"plugin",
			[desc, contextToUse](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			std::cout << "[StudioPluginManager] Factory called - passing context " << contextToUse
				<< " to plugin factory" << std::endl;

			// CRITICAL: Call the plugin's factory function with BOTH EntityManager AND ImGuiContext
			auto view = desc.factory(mgr, contextToUse);

			std::cout << "[StudioPluginManager] Plugin factory returned view: " << (view ? "SUCCESS" : "NULL") << std::endl;
			return view;
		},
			[desc]() -> GUI::ViewMetadata {
			GUI::ViewMetadata meta;
			meta.displayName = desc.name;
			meta.category = desc.category;
			meta.description = "View from plugin: " + desc.name;
			return meta;
		}
		);

		// Store for cleanup
		pluginViews[pluginName].push_back(viewTypeID);
		pluginViewNames[pluginName].push_back(desc.name);

		std::cout << "[StudioPluginManager] Successfully registered view " << desc.name
			<< " with ID " << viewTypeID << " for plugin " << pluginName << std::endl;
		return viewTypeID;
	}

	void StudioPluginManager::cleanupPluginViews(const std::string& pluginName) {
		auto nameIt = pluginViewNames.find(pluginName);
		if (nameIt == pluginViewNames.end()) {
			return;
		}

		std::cout << "[StudioPluginManager] Cleaning up views for plugin: " << pluginName << std::endl;

		for (const std::string& viewName : nameIt->second) {
			std::cout << "[StudioPluginManager] Unregistering view: " << viewName << std::endl;

			// Close all instances of this view type
			viewManager.CloseAllViewsOfType(viewName);

			// Unregister from ViewManager
			viewManager.UnregisterViewType(viewName);

			// Unregister from ViewTypeRegistry
			GUI::ViewTypeRegistry::UnregisterType(viewName);
		}

		// Clean up all plugin views from the source
		viewManager.UnregisterViewSource("plugin");

		pluginViewNames.erase(nameIt);

		// Also clean up ID tracking
		auto idIt = pluginViews.find(pluginName);
		if (idIt != pluginViews.end()) {
			pluginViews.erase(idIt);
		}

		std::cout << "[StudioPluginManager] View cleanup completed for plugin: " << pluginName << std::endl;
	}

} // namespace Plugins