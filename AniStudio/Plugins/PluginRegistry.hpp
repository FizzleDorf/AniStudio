/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 */

#pragma once

 // Forward declarations only - no includes to break circular dependencies
namespace ECS {
	class EntityManager;
	class BaseSystem;
	struct BaseComponent;
	using EntityID = size_t;
}

namespace GUI {
	class ViewManager;
	class BaseView;
	using ViewListID = size_t;
}

#include <functional>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <vector>
#include <string>

namespace Plugin {

	// Forward declaration
	class BasePlugin;

	// Registry for managing plugin-registered components, systems, and views
	class PluginRegistry {
	public:
		// System creation factory function type
		using SystemCreator = std::function<std::shared_ptr<ECS::BaseSystem>(ECS::EntityManager&)>;

		// View creation factory function type  
		using ViewCreator = std::function<std::unique_ptr<GUI::BaseView>(ECS::EntityManager&)>;

		// Component creator function type
		using ComponentCreator = std::function<void(ECS::EntityID)>;

		// Plugin creation factory function type
		using PluginCreator = std::function<BasePlugin*()>;

		// Initialize the registry with references to managers
		// Now takes pointers by reference to ensure we're working with the same instances
		static void Initialize(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr);

		// Set managers directly (for plugins to use the host's managers)
		static void SetManagers(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr);

		// Component registration - now requires managers to be passed in
		template<typename T>
		static void RegisterComponent(const std::string& name, ECS::EntityManager* entityMgr = nullptr) {
			ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: EntityManager not available for component registration!" << std::endl;
				return;
			}

			try {
				// Register with the entity manager's component system
				mgr->RegisterComponentName<T>(name);

				std::cout << "PluginRegistry: Registered component: " << name << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register component " << name
					<< ": " << e.what() << std::endl;
			}
		}

		// System registration - now requires managers to be passed in
		template<typename T>
		static void RegisterSystem(ECS::EntityManager* entityMgr = nullptr) {
			ECS::EntityManager* mgr = entityMgr ? entityMgr : GetEntityManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: EntityManager not available for system registration!" << std::endl;
				return;
			}

			try {
				mgr->RegisterSystem<T>();

				std::cout << "PluginRegistry: Registered system: " << typeid(T).name() << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register system " << typeid(T).name()
					<< ": " << e.what() << std::endl;
			}
		}

		// View registration - now requires managers to be passed in
		template<typename T>
		static void RegisterView(const std::string& name, GUI::ViewManager* viewMgr = nullptr) {
			GUI::ViewManager* mgr = viewMgr ? viewMgr : GetViewManager();
			if (!mgr) {
				std::cerr << "PluginRegistry: ViewManager not available for view registration!" << std::endl;
				return;
			}

			try {
				mgr->RegisterViewType<T>(name);

				std::cout << "PluginRegistry: Registered view: " << name << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << "PluginRegistry: Failed to register view " << name
					<< ": " << e.what() << std::endl;
			}
		}

		// Create and register a view instance - now requires managers to be passed in
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
				vMgr->AddView<T>(viewID, T(*eMgr));
				vMgr->GetView<T>(viewID).Init();

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
		static void RegisterPlugin(const std::string& name, PluginCreator creator) {
			if (s_pluginCreators.find(name) != s_pluginCreators.end()) {
				std::cerr << "PluginRegistry: Plugin " << name << " already registered!" << std::endl;
				return;
			}

			s_pluginCreators[name] = creator;
			std::cout << "PluginRegistry: Registered plugin creator: " << name << std::endl;
		}

		// Get all registered plugin creators
		static const std::unordered_map<std::string, PluginCreator>& GetPluginCreators() {
			return s_pluginCreators;
		}

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
				mgr->AddComponent<T>(entityID);
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
		// Local static storage for this binary's instance
		static ECS::EntityManager* s_entityManager;
		static GUI::ViewManager* s_viewManager;

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