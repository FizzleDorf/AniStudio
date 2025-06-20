#pragma once

#include <imgui.h>

// Only define these if they haven't been defined on the command line
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
#ifdef ANI_CORE_EXPORTS
#define ANI_CORE_API __declspec(dllexport)
#else
#define ANI_CORE_API __declspec(dllimport)
#endif
#else
#define ANI_CORE_API __attribute__((visibility("default")))
#endif
#endif

// Forward declarations
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

namespace Plugin {
	class BasePlugin;
}

// Function pointer types
typedef ECS::EntityManager* (*GetEntityManagerFunc)();
typedef GUI::ViewManager* (*GetViewManagerFunc)();
typedef ImGuiContext* (*GetImGuiContextFunc)();
typedef ImGuiMemAllocFunc(*GetImGuiAllocFunc)();
typedef ImGuiMemFreeFunc(*GetImGuiFreeFunc)();
typedef void* (*GetImGuiUserDataFunc)();

extern "C" {
	// Host-side exported functions (main executable exports these)
	ANI_CORE_API ECS::EntityManager* GetHostEntityManager();
	ANI_CORE_API GUI::ViewManager* GetHostViewManager();
	ANI_CORE_API ImGuiContext* GetHostImGuiContext();
	ANI_CORE_API ImGuiMemAllocFunc GetHostImGuiAllocFunc();
	ANI_CORE_API ImGuiMemFreeFunc GetHostImGuiFreeFunc();
	ANI_CORE_API void* GetHostImGuiUserData();

	// Plugin-side exported functions (plugins export these)
	PLUGIN_API Plugin::BasePlugin* CreatePlugin();
	PLUGIN_API void DestroyPlugin(Plugin::BasePlugin* plugin);
	PLUGIN_API const char* GetPluginName();
	PLUGIN_API const char* GetPluginVersion();

	// UNIFIED: SetManagerGetters function (plugins export this)
	PLUGIN_API void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter
	);
}