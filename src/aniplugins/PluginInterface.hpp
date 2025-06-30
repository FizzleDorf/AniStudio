// PluginInterface.hpp - Simplified interface for plugins to avoid complex includes
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

// Forward declarations only - no complex includes
namespace ECS {
	using EntityID = size_t;
	class EntityManager;
}

namespace GUI {
	using ViewListID = size_t;
	class ViewManager;
}

// Platform-specific plugin export macros
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif

// ============================================================================
// PLUGIN REGISTRY INTERFACE - Simplified access to your existing system
// ============================================================================

namespace Plugin {

	// Function types that match your PluginRegistry
	using ComponentCreateFunc = std::function<void(ECS::EntityManager*, ECS::EntityID)>;
	using ComponentGetFunc = std::function<void*(ECS::EntityManager*, ECS::EntityID)>;
	using ComponentHasFunc = std::function<bool(ECS::EntityManager*, ECS::EntityID)>;
	using ComponentRemoveFunc = std::function<void(ECS::EntityManager*, ECS::EntityID)>;
	using SystemCreateFunc = std::function<void(ECS::EntityManager*)>;
	using ViewCreateFunc = std::function<GUI::ViewListID(GUI::ViewManager*, ECS::EntityManager*)>;

	// ============================================================================
	// PLUGIN BASE CLASS - Simplified version that doesn't depend on heavy includes
	// ============================================================================

	class BasePlugin {
	public:
		BasePlugin() = default;
		virtual ~BasePlugin() = default;

		// Core plugin lifecycle
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
		// Store manager references
		ECS::EntityManager* entityManager = nullptr;
		GUI::ViewManager* viewManager = nullptr;

		// Helper methods that will call into the host's PluginRegistry
		bool RegisterComponent(const std::string& name,
			ComponentCreateFunc createFunc,
			ComponentGetFunc getFunc,
			ComponentHasFunc hasFunc,
			ComponentRemoveFunc removeFunc);

		bool RegisterSystem(const std::string& name, SystemCreateFunc createFunc);
		bool RegisterView(const std::string& name, ViewCreateFunc createFunc);

		// Entity/Component operations that will call into the host's PluginRegistry
		ECS::EntityID CreateEntity();
		void DestroyEntity(ECS::EntityID entity);
		bool IsEntityValid(ECS::EntityID entity);
		bool AddComponent(ECS::EntityID entity, const std::string& componentName);
		void* GetComponent(ECS::EntityID entity, const std::string& componentName);
		bool HasComponent(ECS::EntityID entity, const std::string& componentName);
		void RemoveComponent(ECS::EntityID entity, const std::string& componentName);
		bool CreateSystem(const std::string& systemName);
		GUI::ViewListID CreateView(const std::string& viewName);
		bool HasGUISupport();
	};

	// ============================================================================
	// PLUGIN COMPONENT STORAGE - Simple in-plugin storage for custom components
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
}

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
// PLUGIN HELPER MACROS
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