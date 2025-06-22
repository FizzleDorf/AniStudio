//============================================================================
// PluginAPI.hpp - Plugin API Definitions - CRASH FIXED
//============================================================================

#pragma once

#include <imgui.h>  // MOVE THIS TO TOP

// Platform-specific API macros
#ifndef PLUGIN_API
#ifdef _WIN32
#ifdef PLUGIN_EXPORTS
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllimport)
#endif
#else
#define PLUGIN_API __attribute__((visibility("default")))
#endif
#endif

#ifndef ANI_CORE_API
#ifdef _WIN32
#define ANI_CORE_API __declspec(dllexport)
#else
#define ANI_CORE_API __attribute__((visibility("default")))
#endif
#endif

// Forward declarations
namespace Plugin {
	class BasePlugin;
}

namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

// Function pointer types for plugin interface
typedef Plugin::BasePlugin* (*CreatePluginFunc)();
typedef void(*DestroyPluginFunc)(Plugin::BasePlugin*);
typedef const char* (*GetPluginNameFunc)();
typedef const char* (*GetPluginVersionFunc)();
typedef const char* (*GetPluginDescriptionFunc)();
typedef bool(*GetPluginCanHotReloadFunc)();

// Function pointer types for host interface - USE IMGUI TYPES DIRECTLY
typedef ECS::EntityManager* (*GetEntityManagerFunc)();
typedef GUI::ViewManager* (*GetViewManagerFunc)();
typedef ImGuiContext* (*GetImGuiContextFunc)();
typedef ImGuiMemAllocFunc(*GetImGuiAllocFunc)();  // Use ImGui's typedef directly
typedef ImGuiMemFreeFunc(*GetImGuiFreeFunc)();    // Use ImGui's typedef directly
typedef void* (*GetImGuiUserDataFunc)();

// Required C interface that all plugins must implement
extern "C" {
	// Plugin creation/destruction - REQUIRED
	PLUGIN_API Plugin::BasePlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin);

	// Plugin information - REQUIRED
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();

	// Plugin capabilities - OPTIONAL (defaults provided)
	PLUGIN_API const char* GetPluginDescription();
	PLUGIN_API bool GetPluginCanHotReload();

	// Manager getter setup - OPTIONAL but recommended for proper cross-DLL access
	PLUGIN_API void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter
	);
}

// Host interface functions (declared outside extern "C" for C++ use)
ANI_CORE_API ECS::EntityManager* GetHostEntityManager();
ANI_CORE_API GUI::ViewManager* GetHostViewManager();
ANI_CORE_API ImGuiContext* GetHostImGuiContext();
ANI_CORE_API ImGuiMemAllocFunc GetHostImGuiAllocFunc();
ANI_CORE_API ImGuiMemFreeFunc GetHostImGuiFreeFunc();
ANI_CORE_API void* GetHostImGuiUserData();