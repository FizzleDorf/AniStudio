// AniEngine.hpp - FIXED VERSION
#pragma once

#define ANI_ENGINE_API

// Forward declarations to avoid circular includes
namespace ECS { class EntityManager; }
namespace Plugin { class PluginManager; }

#include <memory>
#include <string>

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

		// CRITICAL FIX: Make this public so StudioCore can access the SAME EntityManager instance
		static ECS::EntityManager& GetEntityManagerImpl();
		static Plugin::PluginManager& GetPluginManagerImpl();

	private:
		// Private implementation - defined in .cpp file
		static bool s_initialized;
		static bool s_running;
	};

}