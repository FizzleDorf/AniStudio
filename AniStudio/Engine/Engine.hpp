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
 * For commercial license iformation, please contact legal@kframe.ai.
 */

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