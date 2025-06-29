// Engine.hpp - FIXED VERSION
#pragma once

#include "AniStudio.hpp" // The AniStudio API also contains the AniEngine
#include "OpenGLWrapper.hpp"
#include "FilePaths.hpp"
#include <memory>

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

		// Dependency accessors - delegate to StudioCore APIs
		ECS::EntityManager &GetEntityManager();
		GUI::ViewManager &GetViewManager();
		Plugin::PluginManager &GetPluginManager();
		GLFWwindow *Window() const { return window; }
		bool Run() const { return run; }

	private:
		Engine();

		// Window/graphics management only
		bool InitializeWindow();
		void CleanupWindow();

		// Window state
		bool run;
		GLFWwindow *window;
		int videoWidth;
		int videoHeight;

		// FPS tracking
		double fpsSum;
		int frameCount;
		double timeElapsed;
	};

	// Global callbacks for GLFW
	void WindowCloseCallback(GLFWwindow *window);
	extern Engine &Core;

} // namespace ANI