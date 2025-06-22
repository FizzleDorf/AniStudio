//============================================================================
// PluginRegistry.cpp - FIXED Plugin Registration System Implementation
//============================================================================

#include "PluginRegistry.hpp"
#include <iostream>

namespace Plugin {

	// Static member definitions
	ECS::EntityManager* PluginRegistry::s_entityManager = nullptr;
	GUI::ViewManager* PluginRegistry::s_viewManager = nullptr;
	std::unordered_map<std::string, PluginCreator> PluginRegistry::s_pluginCreators;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginComponents;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginSystems;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginViews;

	void PluginRegistry::Initialize(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr) {
		std::cout << "PluginRegistry::Initialize called" << std::endl;

		if (!entityMgr || !viewMgr) {
			std::cerr << "PluginRegistry::Initialize - NULL MANAGERS PASSED!" << std::endl;
			return;
		}

		s_entityManager = entityMgr;
		s_viewManager = viewMgr;

		std::cout << "PluginRegistry initialized with EntityManager: " << entityMgr
			<< ", ViewManager: " << viewMgr << std::endl;
	}

	void PluginRegistry::Shutdown() {
		std::cout << "PluginRegistry::Shutdown called" << std::endl;

		// Clear all static data
		s_pluginCreators.clear();
		s_pluginComponents.clear();
		s_pluginSystems.clear();
		s_pluginViews.clear();

		s_entityManager = nullptr;
		s_viewManager = nullptr;

		std::cout << "PluginRegistry shutdown complete" << std::endl;
	}

	void PluginRegistry::RegisterPlugin(const std::string& name, PluginCreator creator) {
		std::cout << "PluginRegistry: Registering plugin: " << name << std::endl;
		s_pluginCreators[name] = creator;
	}

	const std::unordered_map<std::string, PluginCreator>& PluginRegistry::GetPluginCreators() {
		return s_pluginCreators;
	}

	ECS::EntityID PluginRegistry::CreateEntity(ECS::EntityManager* entityMgr) {
		ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
		if (!mgr) {
			std::cerr << "PluginRegistry: EntityManager not available for entity creation!" << std::endl;
			return 0;
		}

		try {
			return mgr->AddNewEntity();
		}
		catch (const std::exception& e) {
			std::cerr << "PluginRegistry: Failed to create entity: " << e.what() << std::endl;
			return 0;
		}
	}

	std::vector<std::string> PluginRegistry::GetRegisteredComponents(ECS::EntityManager* entityMgr) {
		ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
		if (!mgr) {
			std::cerr << "PluginRegistry: EntityManager not available!" << std::endl;
			return {};
		}

		try {
			return mgr->GetAllRegisteredComponentNames();
		}
		catch (const std::exception& e) {
			std::cerr << "PluginRegistry: Failed to get registered components: " << e.what() << std::endl;
			return {};
		}
	}

	ECS::EntityManager* PluginRegistry::GetEntityManager() {
		if (!s_entityManager) {
			std::cerr << "PluginRegistry: EntityManager not initialized!" << std::endl;
		}
		return s_entityManager;
	}

	GUI::ViewManager* PluginRegistry::GetViewManager() {
		if (!s_viewManager) {
			std::cerr << "PluginRegistry: ViewManager not initialized!" << std::endl;
		}
		return s_viewManager;
	}

	void PluginRegistry::CleanupPlugin(const std::string& pluginName) {
		std::cout << "PluginRegistry: Cleaning up plugin: " << pluginName << std::endl;

		// Remove plugin-specific registrations
		s_pluginComponents.erase(pluginName);
		s_pluginSystems.erase(pluginName);
		s_pluginViews.erase(pluginName);
		s_pluginCreators.erase(pluginName);

		std::cout << "PluginRegistry: Cleanup complete for plugin: " << pluginName << std::endl;
	}

} // namespace Plugin