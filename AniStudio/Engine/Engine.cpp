#include "pch.h"
#include "Engine.h"

namespace ANI {

	Engine::Engine() :run(true), window(NULL), videoWidth(SCREEN_WIDTH), videoHeight(SCREEN_HEIGHT) {

		glfwInit();
		//checks if GLFW version is compatible with 3.1 at least otherwise use a later version
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		//always creates the window on the primary monitor
		auto& monitor = *glfwGetVideoMode(glfwGetPrimaryMonitor());

		glfwWindowHint(GLFW_RED_BITS, monitor.redBits);
		glfwWindowHint(GLFW_BLUE_BITS, monitor.blueBits);
		glfwWindowHint(GLFW_GREEN_BITS, monitor.greenBits);
		glfwWindowHint(GLFW_REFRESH_RATE, monitor.refreshRate);

		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE /*GLFW_OPENGL_CORE_PROFILE*/);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "AniStudio", NULL, NULL);
		assert(window && "ERROR : GLFW : Failed to create window!");
		glfwMakeContextCurrent(window);
		
		assert(glewInit() == GLEW_OK && "ERROR : GLEW : Init failed!");
		glewExperimental = GL_TRUE;
	
	}

	Engine::~Engine() {
		glfwTerminate();
	}

	void Engine::Init() {

	}

	void Engine::Update() {

	}

	void Engine::Quit() {
		run = false;
	}
}