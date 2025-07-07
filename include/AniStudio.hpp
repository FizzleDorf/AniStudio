/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
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
#include "GUI.h"
#include "ProjectManager.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define ANI_STUDIO_API

namespace ANI {

	class ANI_STUDIO_API StudioCore {
	public:
		// Constructor/Destructor
		StudioCore();
		~StudioCore();

		// Core lifecycle
		bool Initialize();
		void Shutdown();
		void Update(float deltaTime);
		void Render();

		// Manager access
		ECS::EntityManager& GetEntityManager() { return engineCore.GetEntityManager(); }
		GUI::ViewManager& GetViewManager() { return viewManager; }
		Plugin::PluginManager& GetPluginManager() { return engineCore.GetPluginManager(); }

		// Studio state
		bool IsRunning() const { return running && engineCore.IsRunning(); }
		void SetRunning(bool isRunning) {
			running = isRunning;
			engineCore.SetRunning(isRunning);
		}

		// Window management
		void SetWindowHandle(void* window) { windowHandle = window; }
		void SetImGuiContext(void* context) { imguiContext = context; }

		// Plugin management
		bool LoadPlugin(const std::string& path) { return engineCore.LoadPlugin(path); }
		void LoadDefaultPlugins() { engineCore.LoadDefaultPlugins(); }

	private:
		bool initialized;
		bool running;
		void* windowHandle;
		void* imguiContext;

		EngineCore engineCore;
		GUI::ViewManager viewManager;
		ANI::ProjectManager m_projectManager;
		GUI::ViewListID m_menuBarID;

		// Internal setup
		void RegisterCoreViews();
	};

}