/*
 * AniStudio.hpp - Simplified version without PluginManager
 */
#pragma once

#include "AniEngine.hpp"
#include "GUI.h"
#include "ProjectManager.hpp"
#include "ViewState.hpp"
#include "ImGuiStateUtils.hpp"
#include "WindowState.hpp"
#include "ViewTypes.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory>
#include <set>
#include <unordered_map>

#define ANI_STUDIO_API

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
		void SetImGuiContext(void* context) { imguiContext = context; }

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

		// Standalone views
		std::unique_ptr<GUI::MenuBar> m_menuBar;
		std::unique_ptr<GUI::ProjectManagerView> m_projectManagerView;
		bool m_showProjectManagerView = false;

		// Use existing WindowState utility
		Utils::WindowState m_windowState;

		// Internal setup
		void RegisterCoreViews();
		void SetupProjectCallbacks();

		// Window state management
		void InitializeWindowState();
		void SyncWindowStateFromGLFW();
		void ApplyWindowStateToGLFW();
		std::string GetDefaultWindowStatePath() const;
	};

}