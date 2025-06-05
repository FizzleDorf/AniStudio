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

#include <string>

 // Forward declarations to avoid circular dependencies
namespace ECS {
	class EntityManager;
	class BaseSystem;
	struct BaseComponent;
}

namespace GUI {
	class ViewManager;
	class BaseView;
}

namespace Plugin {

	// Plugin API version for compatibility checking
#define ANISTUDIO_PLUGIN_API_VERSION 1

// Export macros for plugin functions
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

// Plugin metadata structure
	struct PluginInfo {
		std::string name;
		std::string version;
		std::string description;
		std::string author;
		int apiVersion = ANISTUDIO_PLUGIN_API_VERSION;
	};

	// Base interface that all plugins must implement
	class BasePlugin {
	public:
		BasePlugin() = default;
		virtual ~BasePlugin() = default;

		// Core plugin lifecycle methods
		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;
		virtual void Update(float deltaTime) {}

		// Plugin metadata
		virtual PluginInfo GetInfo() const = 0;

		// Optional hook methods that plugins can override
		virtual void OnPluginLoaded() {}
		virtual void OnPluginUnloaded() {}

		// Component and system registration helpers
		// These will be called during plugin initialization
		virtual void RegisterComponents() {}
		virtual void RegisterSystems() {}
		virtual void RegisterViews() {}

		// Menu and UI integration
		virtual void OnMenuRender() {}
		virtual bool HasSettingsUI() const { return false; }
		virtual void RenderSettingsUI() {}

		// Serialization support for plugin data
		virtual std::string SerializeSettings() const { return "{}"; }
		virtual void DeserializeSettings(const std::string& data) {}

	protected:
		// Helper methods for plugin developers
		// These provide access to the core AniStudio systems

		// Register a custom component type
		template<typename T>
		void RegisterComponent(const std::string& name);

		// Register a custom system
		template<typename T>
		void RegisterSystem();

		// Register a custom view
		template<typename T>
		void RegisterView(const std::string& name);

		// Create entities and manage components
		ECS::EntityManager* GetEntityManager();
		GUI::ViewManager* GetViewManager();

		// Utility functions for common plugin operations
		void LogInfo(const std::string& message);
		void LogWarning(const std::string& message);
		void LogError(const std::string& message);

	private:
		// Internal tracking
		bool m_initialized = false;

		friend class PluginManager;
	};

} // namespace Plugin

// Include the template implementations
#include "PluginRegistry.hpp"

namespace Plugin {

	template<typename T>
	void BasePlugin::RegisterComponent(const std::string& name) {
		PluginRegistry::RegisterComponent<T>(name);
	}

	template<typename T>
	void BasePlugin::RegisterSystem() {
		PluginRegistry::RegisterSystem<T>();
	}

	template<typename T>
	void BasePlugin::RegisterView(const std::string& name) {
		PluginRegistry::RegisterView<T>(name);
	}

} // namespace Plugin

// Required plugin interface functions that every plugin DLL must export
extern "C" {
	PLUGIN_API Plugin::BasePlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin);
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();
	PLUGIN_API const char* GetPluginDescription();
	PLUGIN_API int GetPluginAPIVersion();
}

// Convenience macros for plugin developers
#define ANISTUDIO_PLUGIN_MAIN(PluginClass, Name, Version, Description) \
    extern "C" { \
        PLUGIN_API Plugin::BasePlugin* CreatePlugin() { \
            return new PluginClass(); \
        } \
        PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin) { \
            delete plugin; \
        } \
        PLUGIN_API const char* GetPluginName() { \
            return Name; \
        } \
        PLUGIN_API const char* GetPluginVersion() { \
            return Version; \
        } \
        PLUGIN_API const char* GetPluginDescription() { \
            return Description; \
        } \
        PLUGIN_API int GetPluginAPIVersion() { \
            return ANISTUDIO_PLUGIN_API_VERSION; \
        } \
    }