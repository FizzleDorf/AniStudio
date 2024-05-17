#pragma once
#include "pch.h"

namespace ANI {

	//Startup Resolution
	const int SCREEN_WIDTH(1200);
	const int SCREEN_HEIGHT(720);

	class Engine {

	public:
		
		~Engine();
		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;


		static Engine& Ref() {
			static Engine reference;
			return reference;
		}

		void Quit();
		void Update();
		void Init();

		inline const bool Run() const { return run; }
		inline GLFWwindow& Window() { return *window; }
		inline const bool VideoWidth() const { return videoWidth; }
		inline const bool VideoHeight() const { return videoHeight; }

	private:
		Engine();

	private:
		bool run;
		GLFWwindow* window;
		float videoWidth, videoHeight;
	};

	static Engine& Core = Engine::Ref();
}