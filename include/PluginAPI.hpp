#pragma once

#include "ECS.h"
#include <string>
#include <memory>

// Forward declarations to avoid circular includes
namespace GUI {
	class ViewManager;
	using ViewListID = size_t;
}

// Platform-specific plugin export macros
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

namespace Plugin {

	// ============================================================================
	// BASE PLUGIN CLASS - Matches what your PluginManager expects
	// ============================================================================

	class BasePlugin {
	public:
		BasePlugin() = default;
		virtual ~BasePlugin() = default;

		// Core plugin lifecycle - EXACTLY what your PluginManager calls
		virtual bool Initialize(ECS::EntityManager& entityManager, GUI::ViewManager* viewManager = nullptr) = 0;
		virtual void Shutdown() = 0;
		virtual void Update(float deltaTime) {}

		// Plugin information
		virtual const std::string& GetName() const = 0;
		virtual const std::string& GetVersion() const = 0;
		virtual const std::string& GetDescription() const {
			static std::string empty = "No description";
			return empty;
		}

		// Plugin capabilities
		virtual bool HasSettings() const { return false; }
		virtual void ShowSettings() {}
		virtual bool CanReload() const { return true; }

	protected:
		// Store manager references for easy access
		ECS::EntityManager* entityManager = nullptr;
		GUI::ViewManager* viewManager = nullptr;
	};

} // namespace Plugin

// ============================================================================
// REQUIRED C INTERFACE - Every plugin DLL must implement these
// ============================================================================
extern "C" {
	// Plugin creation/destruction - REQUIRED
	PLUGIN_API Plugin::BasePlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin);

	// Plugin information - REQUIRED
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();
	PLUGIN_API const char* GetPluginDescription();

	// Plugin capabilities - OPTIONAL
	PLUGIN_API bool HasPluginSettings();
	PLUGIN_API bool CanPluginReload();
}

// ============================================================================
// PLUGIN HELPER MACROS - For easy plugin creation
// ============================================================================

// Macro to implement the required C interface for a plugin class
#define IMPLEMENT_PLUGIN(PluginClass) \
    static std::unique_ptr<PluginClass> g_pluginInstance; \
    \
    extern "C" { \
        PLUGIN_API Plugin::BasePlugin* CreatePlugin() { \
            g_pluginInstance = std::make_unique<PluginClass>(); \
            return g_pluginInstance.get(); \
        } \
        \
        PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin) { \
            g_pluginInstance.reset(); \
        } \
        \
        PLUGIN_API const char* GetPluginName() { \
            return PluginClass::StaticGetName(); \
        } \
        \
        PLUGIN_API const char* GetPluginVersion() { \
            return PluginClass::StaticGetVersion(); \
        } \
        \
        PLUGIN_API const char* GetPluginDescription() { \
            return PluginClass::StaticGetDescription(); \
        } \
        \
        PLUGIN_API bool HasPluginSettings() { \
            return g_pluginInstance ? g_pluginInstance->HasSettings() : false; \
        } \
        \
        PLUGIN_API bool CanPluginReload() { \
            return g_pluginInstance ? g_pluginInstance->CanReload() : true; \
        } \
    }

// ============================================================================
// PLUGIN COMPONENT STORAGE - Simple helper for plugins
// ============================================================================

template<typename T>
class PluginComponentStorage {
public:
	static std::unordered_map<ECS::EntityID, T>& GetStorage() {
		static std::unordered_map<ECS::EntityID, T> storage;
		return storage;
	}

	static void Add(ECS::EntityID entity, const T& component) {
		GetStorage()[entity] = component;
	}

	static T* Get(ECS::EntityID entity) {
		auto& storage = GetStorage();
		auto it = storage.find(entity);
		return (it != storage.end()) ? &it->second : nullptr;
	}

	static bool Has(ECS::EntityID entity) {
		auto& storage = GetStorage();
		return storage.find(entity) != storage.end();
	}

	static void Remove(ECS::EntityID entity) {
		GetStorage().erase(entity);
	}

	static void Clear() {
		GetStorage().clear();
	}
};