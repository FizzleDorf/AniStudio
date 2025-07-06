#pragma once

#include "AniEngine.hpp"

namespace GUI {
	class ViewManager;
	void ShowMenuBar(void* window, ViewManager& viewManager, ECS::EntityManager& entityManager);
}

#define ANI_STUDIO_API

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nlohmann/json.hpp>

namespace ANI {

	class ANI_STUDIO_API StudioCore {
	public:

		static bool Initialize();
		static void Shutdown();
		static void Update(float deltaTime);
		static void Render();

		static ECS::EntityManager& GetEntityManager();

		static GUI::ViewManager& GetViewManager();

		static Plugin::PluginManager& GetPluginManager();

		static bool IsRunning();
		static void SetRunning(bool running);

		static void SetWindowHandle(void* window);
		static void SetImGuiContext(void* context);

		static bool LoadPlugin(const std::string& path);
		static void UnloadPlugin(const std::string& name);
		static void LoadDefaultPlugins();

	private:

		static bool s_initialized;
		static bool s_running;
		static void* s_windowHandle;
		static void* s_imguiContext;

		static GUI::ViewManager& GetViewManagerImpl();

		// Internal setup
		static void RegisterCoreViews();
	};

	void LoadStyleFromFile(ImGuiStyle& style, const std::string& path);
}