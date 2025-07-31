/*
 * PluginAPI.hpp - Single Unified Plugin API
 * Works for both Engine and Studio - managers decide what to provide
 */

#pragma once

#include <string>

 // Forward declarations
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {

	// Single context struct - managers provide what they can
	struct PluginContext {
		ECS::EntityManager* entityManager = nullptr;  // Always available
		GUI::ViewManager* viewManager = nullptr;      // Only available in Studio
	};

	// Single plugin interface
	class IPlugin {
	public:
		virtual ~IPlugin() = default;
		virtual bool Initialize(PluginContext* context) = 0;
		virtual void Shutdown() = 0;
		virtual void Update(float deltaTime) { /* Optional */ }
		virtual const char* GetName() const = 0;
		virtual const char* GetVersion() const = 0;
		virtual const char* GetDescription() const = 0;
	};
}

// Helper macros
#ifndef PLUGIN_API
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif
#endif

#define IMPLEMENT_PLUGIN(PluginClass, Name, Version, Description) \
    extern "C" { \
        PLUGIN_API Plugin::IPlugin* CreatePlugin() { \
            return new PluginClass(); \
        } \
        PLUGIN_API void DestroyPlugin(Plugin::IPlugin* plugin) { \
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
    }