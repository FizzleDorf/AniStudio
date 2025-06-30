/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y8888 888    888  888 d88" 888 888 d88""88b
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

#include "AniEngine.hpp"
#include <string>
#include <memory>
#include <vector>
#include <functional>

 // Forward declare GUI stuff so engine-only apps don't need to include it
namespace GUI {
	class ViewManager;
	class BaseView;
}

// Forward declare CR stuff
struct cr_plugin;

namespace Plugin {

	struct HostData {
		// Core managers
		ECS::EntityManager* entityManager = nullptr;
		GUI::ViewManager* viewManager = nullptr;  // null for engine-only

		// Plugin context info
		void* userdata = nullptr;
		unsigned int version = 0;

		// Host callbacks for window/input management
		const char* (*get_clipboard_fn)(void*) = nullptr;
		void(*set_clipboard_fn)(void*, const char*) = nullptr;
		void(*set_cursor_pos_fn)(void*, double, double) = nullptr;
		void(*get_cursor_pos_fn)(void*, double*, double*) = nullptr;
		int(*get_window_attrib_fn)(void*, int) = nullptr;
		int(*get_mouse_button_fn)(void*, int) = nullptr;
		void(*set_input_mode_fn)(void*, int, int) = nullptr;

		// Context handles
		void* window_handle = nullptr;
		void* imgui_context = nullptr;
	};

	class Plugin {
	public:
		Plugin() = default;
		virtual ~Plugin() = default;

		// CR lifecycle callbacks - implemented by derived classes
		virtual bool OnLoad(HostData* hostData) = 0;
		virtual bool OnUpdate(HostData* hostData, float deltaTime) = 0;
		virtual void OnUnload(HostData* hostData) = 0;
		virtual void OnClose(HostData* hostData) = 0;

		// Plugin info
		virtual const std::string& GetName() const = 0;
		virtual const std::string& GetVersion() const = 0;
		virtual const std::string& GetDescription() const {
			static std::string empty;
			return empty;
		}

		// Optional capabilities
		virtual bool HasSettings() const { return false; }
		virtual void ShowSettings(HostData* hostData) {} // Only called if GUI available
		virtual void SaveState(HostData* hostData) {}
		virtual void LoadState(HostData* hostData) {}

		// Plugin dependencies
		virtual std::vector<std::string> GetDependencies() const { return {}; }
		virtual std::vector<std::string> GetConflicts() const { return {}; }

	protected:
		// Helper to check if we're in a GUI context
		bool HasGUI(HostData* hostData) const {
			return hostData && hostData->viewManager != nullptr;
		}

		// Convenience registration functions using host managers
		template<typename T>
		void RegisterComponent(HostData* hostData, const std::string& name) {
			if (!hostData || !hostData->entityManager) return;

			auto* entityMgr = hostData->entityManager;
			const ECS::ComponentTypeID typeID = ECS::CompType<T>();
			entityMgr->RegisterComponentByName(name, typeID,
				[entityMgr](ECS::EntityID entity) {
				entityMgr->AddComponent<T>(entity);
			},
				[entityMgr](ECS::EntityID entity) -> ECS::BaseComponent* {
				return entityMgr->HasComponent<T>(entity) ?
					&entityMgr->GetComponent<T>(entity) : nullptr;
			},
				[entityMgr](ECS::EntityID entity) {
				return entityMgr->HasComponent<T>(entity);
			},
				[entityMgr](ECS::EntityID entity) {
				entityMgr->RemoveComponent<T>(entity);
			}
			);
		}

		template<typename T>
		void RegisterSystem(HostData* hostData) {
			if (!hostData || !hostData->entityManager) return;
			hostData->entityManager->RegisterSystem<T>();
		}

		template<typename T>
		void RegisterView(HostData* hostData, const std::string& name) {
			if (!hostData || !hostData->viewManager) return; // Skip if no GUI
			hostData->viewManager->RegisterView<T>(name);
		}

		// Helper to create entities
		ECS::EntityID CreateEntity(HostData* hostData) {
			return (hostData && hostData->entityManager) ?
				hostData->entityManager->AddNewEntity() : 0;
		}
	};

	// Static instance for CR callbacks
	extern Plugin* g_currentPlugin;
}

// CR plugin entry point - all plugins must implement this
extern "C" {
	int cr_main(struct cr_plugin* ctx, enum cr_op operation);
}