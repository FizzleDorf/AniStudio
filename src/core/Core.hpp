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

 /*
 This is the application core that is run in the main loop. please use this as an example for integrating the AniStudio library
 into your own applications. If all you want is the AniEngine library for integrating into your own frontend solution, see the
 AniStudio hpp/cpp for more details. the header API is located in root/include for convenience.
 */

namespace ANI {

	// min window size for first time startups
	const int SCREEN_WIDTH = 1200;
	const int SCREEN_HEIGHT = 720;

	class Core {
	public:

		/*
		singleton instance for keeping one degree of separation from the main thread
		the Core thread will be used as the mainline thread for running the libraries
		this isn't required for implementing the library into your own projects
		*/
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

		// FIXED: Use the correct plugin manager accessor methods
		Plugin::StudioPluginManager& GetStudioPluginManager() { return studioCore.GetStudioPluginManager(); }
		Plugin::EnginePluginManager& GetEnginePluginManager() { return studioCore.GetEnginePluginManager(); }

		GLFWwindow* Window() const { return window; }
		bool Run() const { return run; }

	private:
		Core();

		// Window/graphics management only
		bool InitializeWindow();
		void CleanupWindow();

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