#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations for managers
namespace ECS {
	class EntityManager;
}
namespace GUI {
	class ViewManager;
}

namespace Plugin {

	// Plugin context structure
	struct PluginContext {
		ECS::EntityManager* entityManager;
		GUI::ViewManager* viewManager;
	};

	// Simple plugin interface
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

} // namespace Plugin

// Plugin exports
extern "C" {
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

	PLUGIN_API Plugin::IPlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::IPlugin* plugin);
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();
	PLUGIN_API const char* GetPluginDescription();
}

// Convenience macro for plugin implementation
#define EXPORT_PLUGIN(PluginClass, Name, Version, Description) \
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