/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "8888 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include <cstddef>

 // Platform-specific export macros
#ifdef _WIN32
#ifdef ANI_CORE_EXPORTS
#define ANI_CORE_API __declspec(dllexport)
#else
#define ANI_CORE_API __declspec(dllimport)
#endif
#else
#define ANI_CORE_API __attribute__((visibility("default")))
#endif

// Forward declarations
namespace ECS {
	class EntityManager;
	using EntityID = size_t;
	using ComponentTypeID = size_t;
	using SystemTypeID = size_t;
}

namespace GUI {
	class ViewManager;
	using ViewListID = size_t;
	using ViewTypeID = size_t;
}

// ============================================================================
// ANISTUDIO PLUGIN API - GENERIC MANAGER FUNCTIONS
// ============================================================================

extern "C" {
	// ========================================================================
	// GENERIC ENTITY MANAGER FUNCTIONS
	// ========================================================================

	// Entity management
	ANI_CORE_API ECS::EntityID PluginAddNewEntity(ECS::EntityManager* mgr);
	ANI_CORE_API void PluginDestroyEntity(ECS::EntityManager* mgr, ECS::EntityID entity);
	ANI_CORE_API bool PluginIsEntityValid(ECS::EntityManager* mgr, ECS::EntityID entity);

	// Generic component registration (plugins define their own components)
	ANI_CORE_API void PluginRegisterComponentByName(ECS::EntityManager* mgr, const char* name,
		void(*creator)(ECS::EntityManager*, ECS::EntityID),
		void*(*getter)(ECS::EntityManager*, ECS::EntityID),
		bool(*hasComponent)(ECS::EntityManager*, ECS::EntityID),
		void(*remover)(ECS::EntityManager*, ECS::EntityID));

	// Generic component operations by name
	ANI_CORE_API ECS::ComponentTypeID PluginGetComponentTypeByName(ECS::EntityManager* mgr, const char* name);
	ANI_CORE_API bool PluginHasComponentByName(ECS::EntityManager* mgr, ECS::EntityID entity, const char* name);
	ANI_CORE_API void PluginAddComponentByName(ECS::EntityManager* mgr, ECS::EntityID entity, const char* name);
	ANI_CORE_API void PluginRemoveComponentByName(ECS::EntityManager* mgr, ECS::EntityID entity, const char* name);

	// ========================================================================
	// GENERIC VIEW MANAGER FUNCTIONS  
	// ========================================================================

	// View management
	ANI_CORE_API GUI::ViewListID PluginCreateView(GUI::ViewManager* mgr);
	ANI_CORE_API void PluginDestroyView(GUI::ViewManager* mgr, GUI::ViewListID viewID);

	// Generic view registration (plugins define their own views)
	ANI_CORE_API void PluginRegisterViewByName(GUI::ViewManager* mgr, const char* name, GUI::ViewTypeID typeID);
	ANI_CORE_API GUI::ViewTypeID PluginGetViewTypeByName(GUI::ViewManager* mgr, const char* name);

	// ========================================================================
	// HOST MANAGER ACCESS
	// ========================================================================

	// Host manager accessors - plugins get these through HostData anyway
	ANI_CORE_API ECS::EntityManager* GetHostEntityManager();
	ANI_CORE_API GUI::ViewManager* GetHostViewManager();
	ANI_CORE_API void* GetHostImGuiContext();

	// ========================================================================
	// PLUGIN CALLBACK REGISTRATION
	// ========================================================================

	// Register plugin component factory functions with the host
	ANI_CORE_API void PluginSetComponentFactory(ECS::EntityManager* mgr, const char* name,
		void(*addComponentFunc)(ECS::EntityManager*, ECS::EntityID, const void*),
		void*(*getComponentFunc)(ECS::EntityManager*, ECS::EntityID),
		bool(*hasComponentFunc)(ECS::EntityManager*, ECS::EntityID),
		void(*removeComponentFunc)(ECS::EntityManager*, ECS::EntityID));

	// Register plugin system factory functions with the host  
	ANI_CORE_API void PluginSetSystemFactory(ECS::EntityManager* mgr, const char* name,
		void(*registerSystemFunc)(ECS::EntityManager*));

	// Register plugin view factory functions with the host
	ANI_CORE_API void PluginSetViewFactory(GUI::ViewManager* mgr, const char* name,
		GUI::ViewListID(*createViewFunc)(GUI::ViewManager*, ECS::EntityManager*));
}