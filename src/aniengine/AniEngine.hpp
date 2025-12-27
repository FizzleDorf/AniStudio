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
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#define ANI_ENGINE_API

#include "ECS.h"
#include "EngineContext.hpp"
#include <memory>
#include <string>

 /*
 The ECS backend for AniStudio. The engine contains all the classes and logic for performing
 asset IO, machine learning inference and stored data structures. Components are the structured
 data that systems process asynchronously. Entities are vectors of components for ID and system
 processes. Entities and components can be accessed, added or removed via the EntityManager.
 The EntityManager should only be accessed on the thread it was created on (just like imgui
 and OpenGL).
 */

namespace ANI {

	class ANI_ENGINE_API EngineCore {
	public:
		// Constructor/Destructor
		EngineCore();
		~EngineCore();

		// Core lifecycle
		bool Initialize();
		void Shutdown();
		void Update(float deltaTime);

		// Manager access via context
		ECS::EntityManager& GetEntityManager() {
			if (!context || !context->entityManager) {
				throw std::runtime_error("EngineContext or EntityManager not initialized");
			}
			return *context->entityManager;
		}

		Plugins::PluginManager* GetPluginManager() {
			return context ? context->pluginManager.get() : nullptr;
		}

		// Context access
		std::shared_ptr<EngineContext> GetEngineContext() const { return context; }

		// Create EngineCore with existing context
		static std::unique_ptr<EngineCore> CreateWithContext(std::shared_ptr<EngineContext> existingContext);

		// Engine state
		bool IsRunning() const { return running; }
		void SetRunning(bool isRunning) { running = isRunning; }
		bool IsInitialized() const { return initialized; }

		// Plugin management
		void SetPluginDirectory(const std::string& directory) {
			if (context) {
				context->pluginDirectory = directory;
			}
		}

	private:
		bool initialized;
		bool running;
		std::shared_ptr<EngineContext> context;

		// Component/System registration
		void RegisterCoreComponents();
		void RegisterCoreSystems();

		// Plugin management
		void InitializePlugins();
	};
}