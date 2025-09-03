#include "StudioPluginManager.hpp"
#include "ViewManager.hpp"
#include <iostream>

namespace Plugins {

	// StudioPluginRegistry implementation
	StudioPluginRegistry::StudioPluginRegistry(const std::string& pluginName, StudioPluginManager* manager, GUI::ViewManager& viewMgr)
		: PluginRegistry(pluginName, manager), viewManager(viewMgr), studioManager(manager) {
	}

	GUI::ViewTypeID StudioPluginRegistry::RegisterView(const ViewDescriptor& desc) {
		std::cout << "[StudioPluginRegistry] Registering view: " << desc.name << " for plugin: " << GetCurrentPluginName() << std::endl;
		return studioManager->registerView(GetCurrentPluginName(), desc);
	}

	// StudioPluginManager implementation
	StudioPluginManager::StudioPluginManager(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr)
		: PluginManager(entityMgr), viewManager(viewMgr) {
		std::cout << "[StudioPluginManager] Constructor - studio manager created" << std::endl;
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

			// Create a studio registry for this plugin - includes view support
			StudioPluginRegistry registry(pluginName, this, viewManager);

			// Call OnEngineInit first
			std::cout << "[StudioPluginManager] Calling OnEngineInit with direct registry access" << std::endl;

			if (!plugin.instance->OnEngineInit(entityManager, registry)) {
				std::cerr << "[StudioPluginManager] Plugin engine initialization failed: " << pluginName << std::endl;
				plugin.destroyFunc(plugin.instance);
				plugin.instance = nullptr;
				return false;
			}

			// Then call OnStudioInit
			std::cout << "[StudioPluginManager] Calling OnStudioInit with direct registry access" << std::endl;

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
		std::cout << "[StudioPluginManager] Registering view: " << desc.name << " for plugin: " << pluginName << std::endl;

		// Register with ViewManager using factory
		viewManager.RegisterViewWithFactory(
			desc.name,
			desc.category,
			[desc](ECS::EntityManager& mgr) -> std::unique_ptr<GUI::BaseView> {
			void* rawView = desc.creator(&mgr);
			return std::unique_ptr<GUI::BaseView>(static_cast<GUI::BaseView*>(rawView));
		},
			[desc]() -> GUI::ViewMetadata {
			GUI::ViewMetadata meta;
			meta.displayName = desc.name;
			meta.category = desc.category;
			meta.description = "View from plugin: " + desc.name;
			return meta;
		}
		);

		// Get the view type ID from ViewManager
		GUI::ViewTypeID id = viewManager.GetViewType(desc.name);
		pluginViews[pluginName].push_back(id);

		std::cout << "[StudioPluginManager] Registered view " << desc.name << " with ID " << id << " for plugin " << pluginName << std::endl;
		return id;
	}

	void StudioPluginManager::cleanupPluginViews(const std::string& pluginName) {
		auto it = pluginViews.find(pluginName);
		if (it == pluginViews.end()) return;

		for (GUI::ViewTypeID id : it->second) {
			// Unregister from ViewManager
			viewManager.UnregisterViewSource("plugin");
			std::cout << "[StudioPluginManager] Cleaned up view ID: " << id << std::endl;
		}

		pluginViews.erase(it);
	}

} // namespace Plugins