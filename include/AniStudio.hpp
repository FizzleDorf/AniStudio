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

#define ANI_STUDIO_API

#include "AniEngine.hpp"
#include "GUI.h"
#include "ProjectManager.hpp"
#include "ViewState.hpp"
#include "ImGuiStateUtils.hpp"
#include "WindowState.hpp"
#include "ViewTypes.hpp"
#include "StudioPluginManager.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory>
#include <set>
#include <unordered_map>

 /*
 The AniStudio Library is responsible for the AniEngine instance if you choose to use this
 library. You can access any of the managers from the get functions here. Other than holding
 the AniEngine Instance, the library contains all gui logic and utilities. If you want to use
 another gui solution instead of imgui, you can still access the gui utilities via the
 AniStudio shared library. Otherwise, I would suggest you use the AniEngine shared library
 instead. Commercial licence holders can use the static libs or use code directly.
 */

namespace GUI {
	class MenuBar;
	class ProjectManagerView;
}

namespace ANI {

	class ANI_STUDIO_API StudioCore {
	public:
		// Constructor/Destructor
		StudioCore();
		~StudioCore();

		// Core lifecycle
		bool Initialize();
		void CompleteInitialization();
		void Shutdown();
		void Update(float deltaTime);
		void Render();

		// Manager access
		ECS::EntityManager& GetEntityManager() { return engineCore.GetEntityManager(); }
		GUI::ViewManager& GetViewManager() { return viewManager; }
		ANI::ProjectManager& GetProjectManager() { return m_projectManager; }

		// Studio state
		bool IsRunning() const { return running && engineCore.IsRunning(); }
		void SetRunning(bool isRunning) {
			running = isRunning;
			engineCore.SetRunning(isRunning);
		}

		// Window management
		void SetWindowHandle(void* window);
		void SetImGuiContext(void* context);

		//Workspace management
		void SetActiveWorkspace(GUI::WorkspaceID workspaceID);
		GUI::WorkspaceID GetActiveWorkspace() const;

		// Project event handlers
		void OnProjectLoaded(const std::string& projectPath);
		void OnProjectCreated(const std::string& projectPath);
		void OnProjectClosed();

	private:
		bool initialized;
		bool running;
		bool m_isShuttingDown; // Prevent callbacks during shutdown

		// Pointers to the imgui and glfw instances
		void* windowHandle;
		void* imguiContext;

		// Core systems
		EngineCore engineCore;
		GUI::ViewManager viewManager;
		ANI::ProjectManager m_projectManager;

		// Plugin system
		std::unique_ptr<Plugins::StudioPluginManager> studioPluginManager;

		// Standalone views
		std::unique_ptr<GUI::MenuBar> m_menuBar;
		std::unique_ptr<GUI::ProjectManagerView> m_projectManagerView;
		bool m_showProjectManagerView = false;

		// Use existing WindowState utility
		Utils::WindowState m_windowState;

		// Internal setup
		void RegisterCoreViews();
		void SetupProjectCallbacks();
		void InitializeStudioPlugins();

		// Window state management
		void InitializeWindowState();
		void SyncWindowStateFromGLFW();
		void ApplyWindowStateToGLFW();
		std::string GetDefaultWindowStatePath() const;
	};

}