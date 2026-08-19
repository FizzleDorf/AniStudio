#pragma once
#include "AniStudio.hpp"
#include "OpenGLContextHelper.hpp"
#include "OpenGLWrapper.hpp"
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

		bool Run() const { return m_isRunning; }
		void Init();
		void Update(const float deltaT);
		void Draw();
		void Quit();

		bool IsRunning() const { return m_isRunning; }

		GLFWwindow* GetWindow() const { return m_window; }
		StudioCore& GetStudioCore() { return m_studioCore; }
		ImGuiContext* GetImGuiContext() const { return ImGui::GetCurrentContext(); }

	private:
		Core();

		bool InitializeWindow();
		void CleanupWindow();

		void RegisterEventHandlers();

		bool m_isRunning;
		GLFWwindow* m_window;
		int m_videoWidth;
		int m_videoHeight;

		double m_fpsSum;
		int m_frameCount;
		double m_timeElapsed;

		StudioCore m_studioCore;
	};
}