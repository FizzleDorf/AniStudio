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

#include "BasePlugin.hpp"
#include "PluginRegistry.hpp"
#include <iostream>

namespace Plugin {

	// NOTE: Template function implementations are in the header file
	// Only non-template functions should be implemented here

	ECS::EntityManager* BasePlugin::GetEntityManager() {
		return PluginRegistry::GetEntityManager();
	}

	GUI::ViewManager* BasePlugin::GetViewManager() {
		return PluginRegistry::GetViewManager();
	}

	void BasePlugin::LogInfo(const std::string& message) {
		std::cout << "[PLUGIN INFO] " << message << std::endl;
	}

	void BasePlugin::LogWarning(const std::string& message) {
		std::cout << "[PLUGIN WARNING] " << message << std::endl;
	}

	void BasePlugin::LogError(const std::string& message) {
		std::cerr << "[PLUGIN ERROR] " << message << std::endl;
	}

} // namespace Plugin