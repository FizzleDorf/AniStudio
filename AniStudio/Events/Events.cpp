#include "pch.h"
#include "Events.h"
#include "Engine/Engine.h"

namespace ANI {

	// callback
	void WindowCloseCallback(GLFWwindow* window);

	Events::Events() {};
	Events::~Events() {};

	void Events::Poll() {
		glfwPollEvents();
	}

	void Events::Init() {
		GLFWwindow& window = Core.Window();
		glfwSetWindowCloseCallback(&window, WindowCloseCallback);
	}

	// callback
	void WindowCloseCallback(GLFWwindow* window) {
		ANI::Core.Quit();
	}
}