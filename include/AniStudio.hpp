/*
 * AniStudio.hpp - Enhanced version with view close handling
 */
#pragma once

#include "AniEngine.hpp"
#include "GUI.h"
#include "ProjectManager.hpp"
#include "ImGuiStateUtils.hpp"
#include "WindowState.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <memory>
#include <unordered_map>

#define ANI_STUDIO_API

 // Forward declarations
namespace Plugin {
	class StudioPluginManager;
}

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
		Plugin::StudioPluginManager& GetStudioPluginManager() { return *studioPluginManager; }
		Plugin::EnginePluginManager& GetEnginePluginManager() { return engineCore.GetEnginePluginManager(); }

		// Studio state
		bool IsRunning() const { return running && engineCore.IsRunning(); }
		void SetRunning(bool isRunning) {
			running = isRunning;
			engineCore.SetRunning(isRunning);
		}

		// Window management
		void SetWindowHandle(void* window);
		void SetImGuiContext(void* context) { imguiContext = context; }

		// Plugin management (auto-detects and loads any plugin type)
		bool LoadPlugin(const std::string& path);
		bool UnloadPlugin(const std::string& pluginName);
		std::vector<std::string> GetLoadedPlugins() const;

		// Project event handlers
		void OnProjectLoaded(const std::string& projectPath);
		void OnProjectCreated(const std::string& projectPath);
		void OnProjectClosed();

	private:
		bool initialized;
		bool running;

		// Pointers to the imgui and glfw instances
		void* windowHandle;
		void* imguiContext;

		// Core systems
		EngineCore engineCore;
		GUI::ViewManager viewManager;
		ANI::ProjectManager m_projectManager;

		// Studio plugin manager (supports all plugin types)
		std::unique_ptr<Plugin::StudioPluginManager> studioPluginManager;

		// Standalone views (only ProjectManagerView now - contains the popups)
		std::unique_ptr<GUI::MenuBar> m_menuBar;
		std::unique_ptr<GUI::ProjectManagerView> m_projectManagerView;

		// Use existing WindowState utility
		Utils::WindowState m_windowState;

		// Track active view instances - maps ViewState view instances to ViewManager WorkspaceIDs
		std::unordered_map<GUI::WorkspaceID, GUI::WorkspaceID> m_activeViewInstances;

		// Internal setup
		void RegisterCoreViews();
		void SetupProjectCallbacks();

		// Rendering methods
		void RenderActiveWorkspaceViews();

		// Window state management
		void InitializeWindowState();
		void SyncWindowStateFromGLFW();
		void ApplyWindowStateToGLFW();
		std::string GetDefaultWindowStatePath() const;
	};

}