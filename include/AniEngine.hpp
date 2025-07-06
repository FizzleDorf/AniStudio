// AniEngine.hpp - INSTANCE-BASED, NO MORE STATIC BULLSHIT
#pragma once

#define ANI_ENGINE_API

#include "ECS.h"
#include "PluginManager.hpp"
#include <memory>
#include <string>

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

		// Manager access
		ECS::EntityManager& GetEntityManager() { return entityManager; }
		Plugin::PluginManager& GetPluginManager() { return pluginManager; }

		// Plugin management
		bool LoadPlugin(const std::string& path);
		void LoadDefaultPlugins();

		// Engine state
		bool IsRunning() const { return running; }
		void SetRunning(bool isRunning) { running = isRunning; }

		// Component/System registration
		void RegisterCoreComponents();
		void RegisterCoreSystems();

	private:
		bool initialized;
		bool running;

		ECS::EntityManager entityManager;
		Plugin::PluginManager pluginManager;
	};

}