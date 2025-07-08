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

#include "AniStudio.hpp"
#include "OpenGLWrapper.hpp"
#include "FilePaths.hpp"
#include <memory>

namespace ANI {

	const int SCREEN_WIDTH = 1200;
	const int SCREEN_HEIGHT = 720;

	class Core {
	public:
		static Core& Ref() {
			static Core instance;
			return instance;
		}

		~Core();

		void Init();
		void Update(const float deltatime);
		void Draw();
		void Quit();

		// Dependency accessors - delegate to StudioCore instance
		ECS::EntityManager& GetEntityManager() { return studioCore.GetEntityManager(); }
		GUI::ViewManager& GetViewManager() { return studioCore.GetViewManager(); }
		Plugin::PluginManager& GetPluginManager() { return studioCore.GetPluginManager(); }

		GLFWwindow* Window() const { return window; }
		bool Run() const { return run; }

	private:
		Core();

		// Window/graphics management only
		bool InitializeWindow();
		void CleanupWindow();
		void PerformCleanShutdown();  // NEW - actual cleanup logic

		// Window state
		bool run;
		GLFWwindow* window;
		int videoWidth;
		int videoHeight;

		// FPS tracking
		double fpsSum;
		int frameCount;
		double timeElapsed;

		StudioCore studioCore;
	};

	// Global callbacks for GLFW
	void WindowCloseCallback(GLFWwindow* window);
	extern Core& appCore;

} // namespace ANI