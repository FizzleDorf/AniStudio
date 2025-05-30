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

#include <GL/glew.h>
#include <iostream>

namespace Utils {
	class OpenGLUtils {
	public:
		// Generate OpenGL texture from image data
		static GLuint GenerateTexture(int width, int height, int channels, const unsigned char* data) {
			if (!data || width <= 0 || height <= 0 || channels <= 0) {
				return 0;
			}

			GLuint textureID;
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			// Set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Upload texture data
			GLenum format = GL_RGB;
			if (channels == 1) format = GL_RED;
			else if (channels == 3) format = GL_RGB;
			else if (channels == 4) format = GL_RGBA;

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			// Check for OpenGL errors
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				std::cerr << "OpenGL texture creation error: " << error << std::endl;
				glDeleteTextures(1, &textureID);
				return 0;
			}

			glBindTexture(GL_TEXTURE_2D, 0);
			return textureID;
		}

		// Delete OpenGL texture
		static void DeleteTexture(GLuint textureID) {
			if (textureID != 0 && glIsTexture(textureID)) {
				glDeleteTextures(1, &textureID);
			}
		}

		// Check if texture is valid
		static bool IsValidTexture(GLuint textureID) {
			return textureID != 0 && glIsTexture(textureID);
		}
	};
}