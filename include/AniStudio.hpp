// AniStudio.hpp - FIXED VERSION
#pragma once

// Include the core engine API - this gives us ALL the ECS functionality
#include "AniEngine.hpp"

// Forward declarations to avoid circular includes
namespace GUI {
	class ViewManager;
	void ShowMenuBar(void* window, ViewManager& viewManager, ECS::EntityManager& entityManager);
}

#define ANI_STUDIO_API

// GUI and ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

// ============================================================================
// STUDIO CORE - Main Studio Management Class
// ============================================================================

namespace ANI {

	class ANI_STUDIO_API StudioCore {
	public:
		// Core lifecycle
		static bool Initialize();
		static void Shutdown();
		static void Update(float deltaTime);
		static void Render();

		// Manager access
		static ECS::EntityManager& GetEntityManager();
		static GUI::ViewManager& GetViewManager();
		static Plugin::PluginManager& GetPluginManager();

		// Studio state
		static bool IsRunning();
		static void SetRunning(bool running);

		// Window management
		static void SetWindowHandle(void* window);
		static void SetImGuiContext(void* context);

		// Plugin management
		static bool LoadPlugin(const std::string& path);
		static void UnloadPlugin(const std::string& name);
		static void LoadDefaultPlugins();

	private:
		// Private implementation - defined in .cpp file
		static bool s_initialized;
		static bool s_running;
		static void* s_windowHandle;
		static void* s_imguiContext;

		// These will be implemented as stack-allocated statics in the .cpp file
		static ECS::EntityManager& GetEntityManagerImpl();
		static GUI::ViewManager& GetViewManagerImpl();
		static Plugin::PluginManager& GetPluginManagerImpl();

		// Internal setup
		static void RegisterCoreViews();
		static void CreateCoreViews();
	};
}