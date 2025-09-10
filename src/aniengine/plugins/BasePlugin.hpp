#pragma once
#include "PluginRegistry.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugins {

	class BasePlugin {
	public:
		BasePlugin(const std::string& name, const std::string& version = "1.0.0")
			: name(name), version(version) {}
		virtual ~BasePlugin() = default;

		// Plugin info
		const std::string& GetName() const { return name; }
		const std::string& GetVersion() const { return version; }
		bool IsInitialized() const { return initialized; }

		// Internal state management - called by PluginManager
		void SetInitialized(bool initialized) { initialized = initialized; }

		// Lifecycle - implement these in your plugin
		virtual bool OnEngineInit(ECS::EntityManager& entityMgr, IPluginRegistry& registry) = 0;
		virtual bool OnStudioInit(ECS::EntityManager& entityMgr, GUI::ViewManager& viewMgr, IPluginRegistry& registry) { return true; }
		virtual void OnShutdown() {}
		virtual void OnUpdate(float deltaTime) {}

	protected:
		void LogInfo(const std::string& msg) const {
			std::cout << "[" << name << "] " << msg << std::endl;
		}

		void LogError(const std::string& msg) const {
			std::cerr << "[" << name << "] ERROR: " << msg << std::endl;
		}

	private:
		std::string name;
		std::string version;
		bool initialized = false;

		// Track what this plugin registered for cleanup
		std::vector<ECS::ComponentTypeID> registeredComponents;
		std::vector<ECS::SystemTypeID> registeredSystems;
		std::vector<GUI::ViewTypeID> registeredViews;

		friend class PluginManager;
	};

} // namespace Plugins

// Plugin export macros
#define ENGINE_PLUGIN_EXPORT extern "C" __declspec(dllexport)