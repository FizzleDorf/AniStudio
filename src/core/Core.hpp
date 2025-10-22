#pragma once
#include "AniStudio.hpp"
#include "OpenGLContextHelper.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace ANI {

	constexpr int SCREEN_WIDTH = 1280;
	constexpr int SCREEN_HEIGHT = 720;

	class Core {
	public:
		~Core();
		Core(const Core &) = delete;
		Core &operator=(const Core &) = delete;

		static Core &Ref() {
			static Core instance;
			return instance;
		}

		bool Run() const { return run; }
		void Init();
		void Update(const float deltaT);
		void Draw();
		void Quit();

		bool IsRunning() const { return run; }
		GLFWwindow* GetWindow() const { return window; }
		StudioCore& GetStudioCore() { return studioCore; }

		ImGuiContext* GetImGuiContext() const { return ImGui::GetCurrentContext(); }

	private:
		Core();

		bool InitializeWindow();
		void CleanupWindow();

		void RegisterEventHandlers();

		bool run;
		GLFWwindow* window;
		int videoWidth;
		int videoHeight;

		double fpsSum;
		int frameCount;
		double timeElapsed;

		StudioCore studioCore;
	};

} // namespace ANI