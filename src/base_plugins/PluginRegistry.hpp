//============================================================================
// PluginRegistry.hpp - Plugin Registration System  
//============================================================================

#pragma once

#include "PluginAPI.hpp"
#include "ECS.h"
#include "GUI.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <iostream>

namespace Plugin {

	// Plugin creator function type
	typedef std::function<BasePlugin*()> PluginCreator;

	/**
	 * Central registry for plugin components, systems, and views
	 * Handles cross-DLL registration and cleanup
	 */
	class PluginRegistry {
	public:
		// Public access to static members
		static ECS::EntityManager* s_entityManager;
		static GUI::ViewManager* s_viewManager;

		// Initialize the registry with manager references
		static void Initialize(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr);
		static void Shutdown();

		// Component registration
		template<typename T>
		static void RegisterComponent(const std::string& name, ECS::EntityManager* entityMgr = nullptr) {
			ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: EntityManager not available for component registration!" << std::endl;
				return;
			}

			try {
				// Register with the entity manager's component system
				mgr->template RegisterComponentName<T>(name);
				std::cout << "PluginRegistry: Registered component: " << name << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register component " << name
					<< ": " << e.what() << std::endl;
			}
		}

		// System registration
		template<typename T>
		static void RegisterSystem(ECS::EntityManager* entityMgr = nullptr) {
			ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: EntityManager not available for system registration!" << std::endl;
				return;
			}

			try {
				mgr->template RegisterSystem<T>();
				std::cout << "PluginRegistry: Registered system: " << typeid(T).name() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register system " << typeid(T).name()
					<< ": " << e.what() << std::endl;
			}
		}

		// View registration
		template<typename T>
		static void RegisterView(const std::string& name, GUI::ViewManager* viewMgr = nullptr) {
			GUI::ViewManager* mgr = viewMgr ? viewMgr : GetViewManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: ViewManager not available for view registration!" << std::endl;
				return;
			}

			try {
				mgr->template RegisterViewType<T>(name);
				std::cout << "PluginRegistry: Registered view: " << name << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register view " << name
					<< ": " << e.what() << std::endl;
			}
		}

		// Create and register a view instance
		template<typename T>
		static GUI::ViewListID CreateView(const std::string& name,
			ECS::EntityManager* entityMgr = nullptr,
			GUI::ViewManager* viewMgr = nullptr) {
			ECS::EntityManager* eMgr = entityMgr ? entityMgr : GetEntityManager();
			GUI::ViewManager* vMgr = viewMgr ? viewMgr : GetViewManager();

			if (!vMgr || !eMgr) {
				std::cerr << "PluginRegistry: Managers not available for view creation! EntityMgr: "
					<< eMgr << ", ViewMgr: " << vMgr << std::endl;
				return 0;
			}

			try {
				auto viewID = vMgr->CreateView();
				vMgr->template AddView<T>(viewID, T(*eMgr));
				vMgr->template GetView<T>(viewID).Init();

				std::cout << "PluginRegistry: Created view instance: " << name << std::endl;
				return viewID;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to create view " << name
					<< ": " << e.what() << std::endl;
				return 0;
			}
		}

		// Plugin registration (for auto-registration at startup)
		static void RegisterPlugin(const std::string& name, PluginCreator creator);

		// Get all registered plugin creators
		static const std::unordered_map<std::string, PluginCreator>& GetPluginCreators();

		// Create an entity with specified components
		static ECS::EntityID CreateEntity(ECS::EntityManager* entityMgr = nullptr);

		// Add component to entity by name
		template<typename T>
		static void AddComponentToEntity(ECS::EntityID entityID, ECS::EntityManager* entityMgr = nullptr) {
			ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: EntityManager not available!" << std::endl;
				return;
			}

			try {
				mgr->template AddComponent<T>(entityID);
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to add component to entity " << entityID
					<< ": " << e.what() << std::endl;
			}
		}

		// Get registered component names
		static std::vector<std::string> GetRegisteredComponents(ECS::EntityManager* entityMgr = nullptr);

		// Utility functions for plugins - these will use the callback system
		static ECS::EntityManager* GetEntityManager();
		static GUI::ViewManager* GetViewManager();

		// Plugin cleanup - remove all registered items from a specific plugin
		static void CleanupPlugin(const std::string& pluginName);

	private:
		// Storage for plugin creators (for auto-registration)
		static std::unordered_map<std::string, PluginCreator> s_pluginCreators;

		// Storage for plugin-specific registrations (for cleanup)
		static std::unordered_map<std::string, std::vector<std::string>> s_pluginComponents;
		static std::unordered_map<std::string, std::vector<std::string>> s_pluginSystems;
		static std::unordered_map<std::string, std::vector<std::string>> s_pluginViews;
	};

	// Helper functions for cross-binary manager access
	ECS::EntityManager* GetHostEntityManagerViaPointer();
	GUI::ViewManager* GetHostViewManagerViaPointer();

} // namespace Plugin