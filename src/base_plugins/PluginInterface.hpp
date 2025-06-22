//============================================================================
// PluginInterface.hpp - Plugin Interface Definitions
//============================================================================

#pragma once

#include "PluginAPI.hpp"

namespace Plugin {

	// Function to set manager getters for plugins
	void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter
	);

	// Helper functions for cross-binary manager access - DECLARATIONS
	ECS::EntityManager* GetHostEntityManagerViaPointer();
	GUI::ViewManager* GetHostViewManagerViaPointer();
	ImGuiContext* GetHostImGuiContextViaPointer();

} // namespace Plugin

// Plugin-side function to receive manager getters from host
extern "C" {
	PLUGIN_API void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter
	);
}