#include "pch.h"
#include "../Core/Ani.h"

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