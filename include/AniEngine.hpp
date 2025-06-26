/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y8888 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

 // FIXED: Since you're building as static library, remove all DLL export nonsense
#define ANI_ENGINE_API

// Include the existing ECS system files
#include "ECS.h"
#include "PluginManager.hpp"
#include "utils.h"
#include "components.h"
#include "systems.h"

// Core includes
#include <memory>
#include <string>
#include <filesystem>
#include <iostream>

namespace ANI {

	class ANI_ENGINE_API EngineCore {
	public:
		// Core lifecycle
		static bool Initialize();
		static void Shutdown();
		static void Update(float deltaTime);

		// Manager access
		static ECS::EntityManager& GetEntityManager();
		static Plugin::PluginManager& GetPluginManager();

		// Plugin management
		static bool LoadPlugin(const std::string& path);
		static void LoadDefaultPlugins();

		// Engine state
		static bool IsRunning();
		static void SetRunning(bool running);

		// Internal setup - PUBLIC so StudioCore can use them
		static void RegisterCoreComponents(ECS::EntityManager& mgr);
		static void RegisterCoreSystems(ECS::EntityManager& mgr);

	private:
		// FIXED: Static members without any DLL export decorations
		static std::unique_ptr<ECS::EntityManager> s_entityManager;
		static std::unique_ptr<Plugin::PluginManager> s_pluginManager;
		static bool s_initialized;
		static bool s_running;
	};

}