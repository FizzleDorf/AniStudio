#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace ANI {

	class OpenGLContextHelper {
	public:
		// Verify that we have a valid OpenGL context current
		static bool VerifyContext() {
			GLFWwindow* currentContext = glfwGetCurrentContext();
			if (!currentContext) {
				std::cerr << "[OpenGLContextHelper] ERROR: No OpenGL context current!" << std::endl;
				return false;
			}

			// Verify OpenGL is functioning
			const GLubyte* renderer = glGetString(GL_RENDERER);
			const GLubyte* version = glGetString(GL_VERSION);

			if (!renderer || !version) {
				std::cerr << "[OpenGLContextHelper] ERROR: OpenGL not properly initialized!" << std::endl;
				return false;
			}

			return true;
		}

		// Check if texture operations are safe
		static bool CanCreateTextures() {
			if (!VerifyContext()) {
				return false;
			}

			// Additional checks for texture support
			GLint maxTextureSize;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

			if (maxTextureSize <= 0) {
				std::cerr << "[OpenGLContextHelper] ERROR: No texture support detected!" << std::endl;
				return false;
			}

			return true;
		}
	};

} // namespace ANI