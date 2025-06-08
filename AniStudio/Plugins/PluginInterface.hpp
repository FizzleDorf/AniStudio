#pragma once

#include <imgui.h>

// Forward declarations
namespace ECS {
	class EntityManager;
}

namespace GUI {
	class ViewManager;
}

// Function pointer types
typedef ECS::EntityManager* (*GetEntityManagerFunc)();
typedef GUI::ViewManager* (*GetViewManagerFunc)();
typedef ImGuiContext* (*GetImGuiContextFunc)();
typedef ImGuiMemAllocFunc(*GetImGuiAllocFunc)();
typedef ImGuiMemFreeFunc(*GetImGuiFreeFunc)();
typedef void* (*GetImGuiUserDataFunc)();

namespace Plugin {
	// Function to set the manager getters
	void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter);

	// Helper functions to access managers via function pointers
	ECS::EntityManager* GetHostEntityManagerViaPointer();
	GUI::ViewManager* GetHostViewManagerViaPointer();
	ImGuiContext* GetHostImGuiContextViaPointer();
}