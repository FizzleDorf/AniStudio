/*
 * FIXED PluginInterface.cpp - Host-side implementation
 * - No PLUGIN_API on functions (this is the HOST, not a plugin)
 * - Properly implements the plugin interface for cross-DLL communication
 */

#include "PluginInterface.hpp"
#include "PluginRegistry.hpp"
#include "EntityManager.hpp"
#include "ViewManager.hpp"
#include <iostream>

 // Global function pointers for cross-binary access
static GetEntityManagerFunc g_entityManagerGetter = nullptr;
static GetViewManagerFunc g_viewManagerGetter = nullptr;
static GetImGuiContextFunc g_contextGetter = nullptr;
static GetImGuiAllocFunc g_allocGetter = nullptr;
static GetImGuiFreeFunc g_freeGetter = nullptr;
static GetImGuiUserDataFunc g_userDataGetter = nullptr;

// Helper functions for PluginRegistry to access the function pointers
namespace Plugin {
	ECS::EntityManager* GetHostEntityManagerViaPointer() {
		if (g_entityManagerGetter) {
			auto* mgr = g_entityManagerGetter();
			std::cout << "GetHostEntityManagerViaPointer via getter - returning: " << mgr << std::endl;
			return mgr;
		}
		std::cout << "GetHostEntityManagerViaPointer - no getter function available" << std::endl;
		return nullptr;
	}

	GUI::ViewManager* GetHostViewManagerViaPointer() {
		if (g_viewManagerGetter) {
			auto* mgr = g_viewManagerGetter();
			std::cout << "GetHostViewManagerViaPointer via getter - returning: " << mgr << std::endl;
			return mgr;
		}
		std::cout << "GetHostViewManagerViaPointer - no getter function available" << std::endl;
		return nullptr;
	}

	ImGuiContext* GetHostImGuiContextViaPointer() {
		if (g_contextGetter) {
			auto* context = g_contextGetter();
			std::cout << "GetHostImGuiContextViaPointer via getter - returning: " << context << std::endl;
			return context;
		}
		std::cout << "GetHostImGuiContextViaPointer - no getter function available" << std::endl;
		return nullptr;
	}

	// CRITICAL: Function to set the manager getters - this stores them for plugins to use
	void SetManagerGetters(
		GetEntityManagerFunc entityGetter,
		GetViewManagerFunc viewGetter,
		GetImGuiContextFunc contextGetter,
		GetImGuiAllocFunc allocGetter,
		GetImGuiFreeFunc freeGetter,
		GetImGuiUserDataFunc userDataGetter) {

		std::cout << "Plugin::SetManagerGetters called with all getters" << std::endl;

		g_entityManagerGetter = entityGetter;
		g_viewManagerGetter = viewGetter;
		g_contextGetter = contextGetter;
		g_allocGetter = allocGetter;
		g_freeGetter = freeGetter;
		g_userDataGetter = userDataGetter;

		std::cout << "Manager getters set successfully in Plugin namespace" << std::endl;
	}
}

// NOTE: NO extern "C" SetManagerGetters here - that's only in the plugin DLL!