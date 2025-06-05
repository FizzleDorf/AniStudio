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

#include "PluginRegistry.hpp"
#include <iostream>

namespace Plugin {

	// Static member definitions
	ECS::EntityManager* PluginRegistry::s_entityManager = nullptr;
	GUI::ViewManager* PluginRegistry::s_viewManager = nullptr;
	std::unordered_map<std::string, PluginRegistry::PluginCreator> PluginRegistry::s_pluginCreators;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginComponents;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginSystems;
	std::unordered_map<std::string, std::vector<std::string>> PluginRegistry::s_pluginViews;

	// Registry for managing plugin-registered components, systems, and views
	void PluginRegistry::Initialize(ECS::EntityManager* entityMgr, GUI::ViewManager* viewMgr) {
		s_entityManager = entityMgr;
		s_viewManager = viewMgr;
		std::cout << "PluginRegistry initialized" << std::endl;
	}

	ECS::EntityID PluginRegistry::CreateEntity() {
		if (!s_entityManager) {
			std::cerr << "PluginRegistry: EntityManager not initialized!" << std::endl;
			return 0;
		}

		return s_entityManager->AddNewEntity();
	}

	std::vector<std::string> PluginRegistry::GetRegisteredComponents() {
		if (!s_entityManager) {
			return {};
		}

		return s_entityManager->GetAllRegisteredComponentNames();
	}

	ECS::EntityManager* PluginRegistry::GetEntityManager() {
		return s_entityManager;
	}

	GUI::ViewManager* PluginRegistry::GetViewManager() {
		return s_viewManager;
	}

	void PluginRegistry::CleanupPlugin(const std::string& pluginName) {
		// This would be used to track and cleanup plugin-specific registrations
		// For now, we'll just log the cleanup
		std::cout << "PluginRegistry: Cleaning up plugin: " << pluginName << std::endl;
	}

} // namespace Plugin