/*
 * COMPLETELY FIXED PluginRegistry.cpp
 * - Ensures plugins use host managers, not their own
 * - Proper fallback to function pointers for cross-DLL access
 * - Debug output to track manager usage
 */

#include "PluginRegistry.hpp"
#include "PluginInterface.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include <iostream>

namespace Plugin {

	// Static member definitions for this binary's instance
	ECS::EntityManager* PluginRegistry::s_entityManager = nullptr;
	GUI::ViewManager* PluginRegistry::s_viewManager = nullptr;
	std::unordered_map<std::string, PluginRegistry::PluginCreator> PluginRegistry::s_pluginCreators;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginComponents;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginSystems;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginViews;

	// Registry for managing plugin-registered components, systems, and views
	void PluginRegistry::Initialize(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr) {
		std::cout << "PluginRegistry::Initialize called with EntityManager: " << entityMgr
			<< ", ViewManager: " << viewMgr << std::endl;

		s_entityManager = entityMgr;
		s_viewManager = viewMgr;

		std::cout << "PluginRegistry initialized - EntityManager: " << s_entityManager
			<< ", ViewManager: " << s_viewManager << std::endl;
	}

	void PluginRegistry::SetManagers(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr) {
		std::cout << "PluginRegistry::SetManagers called with EntityManager: " << entityMgr
			<< ", ViewManager: " << viewMgr << std::endl;

		s_entityManager = entityMgr;
		s_viewManager = viewMgr;
	}

	ECS::EntityID PluginRegistry::CreateEntity(ECS::EntityManager* entityMgr) {
		ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
		std::cout << "PluginRegistry::CreateEntity called - using EntityManager: " << mgr << std::endl;

		if (!mgr) {
			std::cerr << "PluginRegistry: EntityManager not available!" << std::endl;
			return 0;
		}

		return mgr->AddNewEntity();
	}

	std::vector<std::string> PluginRegistry::GetRegisteredComponents(ECS::EntityManager* entityMgr) {
		ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
		if (!mgr) {
			return {};
		}

		return mgr->GetAllRegisteredComponentNames();
	}

	ECS::EntityManager* PluginRegistry::GetEntityManager() {
		// CRITICAL: First try the function pointer (for plugins accessing main exe managers)
		if (auto mgr = GetHostEntityManagerViaPointer()) {
			std::cout << "PluginRegistry::GetEntityManager via getter - returning HOST: " << mgr << std::endl;
			return mgr;
		}

		// Fallback to local static (for main exe)
		std::cout << "PluginRegistry::GetEntityManager via static - returning LOCAL: " << s_entityManager << std::endl;
		return s_entityManager;
	}

	GUI::ViewManager* PluginRegistry::GetViewManager() {
		// CRITICAL: First try the function pointer (for plugins accessing main exe managers)
		if (auto mgr = GetHostViewManagerViaPointer()) {
			std::cout << "PluginRegistry::GetViewManager via getter - returning HOST: " << mgr << std::endl;
			return mgr;
		}

		// Fallback to local static (for main exe)
		std::cout << "PluginRegistry::GetViewManager via static - returning LOCAL: " << s_viewManager << std::endl;
		return s_viewManager;
	}

	void PluginRegistry::CleanupPlugin(const std::string& pluginName) {
		// This would be used to track and cleanup plugin-specific registrations
		// For now, we'll just log the cleanup
		std::cout << "PluginRegistry: Cleaning up plugin: " << pluginName << std::endl;
	}

} // namespace Plugin