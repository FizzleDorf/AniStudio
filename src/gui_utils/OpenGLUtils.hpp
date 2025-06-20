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
		// Generate OpenGL texture from image data - MAIN THREAD ONLY!
		// This function contains OpenGL calls and MUST be called from the main thread
		// that has the OpenGL context. Do NOT call from worker threads.
		static GLuint GenerateTexture(int width, int height, int channels, const unsigned char* data) {
			// Validate input parameters
			if (!data) {
				std::cerr << "GenerateTexture: Input data is null" << std::endl;
				return 0;
			}

			if (width <= 0 || height <= 0) {
				std::cerr << "GenerateTexture: Invalid dimensions: " << width << "x" << height << std::endl;
				return 0;
			}

			if (channels <= 0 || channels > 4) {
				std::cerr << "GenerateTexture: Invalid channel count: " << channels << std::endl;
				return 0;
			}

			GLuint textureID = 0;

			try {
				// Generate texture ID
				glGenTextures(1, &textureID);
				if (textureID == 0) {
					GLenum error = glGetError();
					std::cerr << "GenerateTexture: glGenTextures failed with error: " << error << std::endl;
					return 0;
				}

				// Bind the texture
				glBindTexture(GL_TEXTURE_2D, textureID);
				GLenum bindError = glGetError();
				if (bindError != GL_NO_ERROR) {
					std::cerr << "GenerateTexture: glBindTexture failed with error: " << bindError << std::endl;
					glDeleteTextures(1, &textureID);
					return 0;
				}

				// Set texture parameters with error checking
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				// Check for errors in texture parameters
				GLenum paramError = glGetError();
				if (paramError != GL_NO_ERROR) {
					std::cerr << "GenerateTexture: Setting texture parameters failed with error: " << paramError << std::endl;
					glDeleteTextures(1, &textureID);
					return 0;
				}

				// Determine the format based on the number of channels
				GLenum format;
				GLenum internalFormat;
				switch (channels) {
				case 1:
					format = GL_RED;
					internalFormat = GL_RED;
					break;
				case 3:
					format = GL_RGB;
					internalFormat = GL_RGB;
					break;
				case 4:
					format = GL_RGBA;
					internalFormat = GL_RGBA;
					break;
				default:
					// Unsupported format (should never reach here due to validation above)
					glDeleteTextures(1, &textureID);
					return 0;
				}

				// Upload image data with error checking
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

				GLenum texError = glGetError();
				if (texError != GL_NO_ERROR) {
					std::cerr << "GenerateTexture: glTexImage2D failed with error: " << texError
						<< " for dimensions " << width << "x" << height
						<< ", channels: " << channels << std::endl;
					glDeleteTextures(1, &textureID);
					return 0;
				}

				// Generate mipmaps with error checking
				glGenerateMipmap(GL_TEXTURE_2D);

				GLenum mipError = glGetError();
				if (mipError != GL_NO_ERROR) {
					std::cerr << "GenerateTexture: glGenerateMipmap failed with error: " << mipError << std::endl;
					// Continue despite mipmap error - texture might still be usable
				}

				// Unbind the texture
				glBindTexture(GL_TEXTURE_2D, 0);

				std::cout << "GenerateTexture: Successfully created texture ID " << textureID
					<< " with dimensions " << width << "x" << height
					<< ", channels: " << channels << std::endl;

				return textureID;
			}
			catch (const std::exception& e) {
				std::cerr << "GenerateTexture: Exception: " << e.what() << std::endl;
				if (textureID != 0) {
					glDeleteTextures(1, &textureID);
				}
				return 0;
			}
			catch (...) {
				std::cerr << "GenerateTexture: Unknown exception" << std::endl;
				if (textureID != 0) {
					glDeleteTextures(1, &textureID);
				}
				return 0;
			}
		}

		// Delete OpenGL texture - MAIN THREAD ONLY!
		static void DeleteTexture(GLuint& textureID) {
			if (textureID != 0) {
				glDeleteTextures(1, &textureID);
				textureID = 0;
			}
		}

		// Check if texture is valid
		static bool IsValidTexture(GLuint textureID) {
			return textureID != 0 && glIsTexture(textureID);
		}

		// Update texture data (useful for video frames) - MAIN THREAD ONLY!
		static bool UpdateTexture(GLuint textureID, int width, int height, int channels, const unsigned char* data) {
			if (!IsValidTexture(textureID) || !data) {
				return false;
			}

			// Determine format
			GLenum format = GL_RGB;
			if (channels == 1) format = GL_RED;
			else if (channels == 3) format = GL_RGB;
			else if (channels == 4) format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				std::cerr << "UpdateTexture: glTexSubImage2D failed with error: " << error << std::endl;
				glBindTexture(GL_TEXTURE_2D, 0);
				return false;
			}

			glBindTexture(GL_TEXTURE_2D, 0);
			return true;
		}

		// Create texture with specific parameters - MAIN THREAD ONLY!
		static GLuint CreateTextureWithParams(int width, int height, GLenum internalFormat, GLenum format,
			GLenum type, const void* data, GLint minFilter = GL_LINEAR,
			GLint magFilter = GL_LINEAR, GLint wrapS = GL_CLAMP_TO_EDGE,
			GLint wrapT = GL_CLAMP_TO_EDGE) {
			GLuint textureID = 0;
			glGenTextures(1, &textureID);

			if (textureID == 0) {
				return 0;
			}

			glBindTexture(GL_TEXTURE_2D, textureID);

			// Set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

			// Upload texture data
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);

			// Check for errors
			GLenum error = glGetError();
			if (error != GL_NO_ERROR) {
				std::cerr << "CreateTextureWithParams: OpenGL error " << error << std::endl;
				glDeleteTextures(1, &textureID);
				glBindTexture(GL_TEXTURE_2D, 0);
				return 0;
			}

			glBindTexture(GL_TEXTURE_2D, 0);
			return textureID;
		}
	};
}