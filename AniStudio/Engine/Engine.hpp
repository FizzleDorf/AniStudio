#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "Base/ViewManager.hpp"
#include "ECS.h"
#include "GUI.h"
#include "../Plugins/PluginManager.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace ECS;
using namespace GUI;
using namespace Plugin;

namespace ANI {

	const int SCREEN_WIDTH = 1200;
	const int SCREEN_HEIGHT = 720;

	class Engine {
	public:
		static Engine &Ref() {
			static Engine instance;
			return instance;
		}

		~Engine();

		void Init();
		void Update(const float deltatime);
		void Draw();
		void Quit();

		// Dependency accessors
		EntityManager &GetEntityManager() { return mgr; }
		ViewManager &GetViewManager() { return viewManager; }
		PluginManager &GetPluginManager() { return pluginManager; }
		GLFWwindow *Window() const { return window; }
		bool Run() const { return run; }

	private:
		Engine();

		// Core dependencies
		EntityManager mgr;
		ViewManager viewManager;
		PluginManager pluginManager{ mgr, viewManager };
		bool run;
		GLFWwindow *window;
		int videoWidth;
		int videoHeight;
		double fpsSum;
		int frameCount;
		double timeElapsed;
	};

	void WindowCloseCallback(GLFWwindow *window);
	extern Engine &Core;

} // namespace ANI

#endif // ENGINE_HPP