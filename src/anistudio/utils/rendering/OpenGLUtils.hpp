#pragma once

#include "OpenGLWrapper.hpp"
#include <iostream>

namespace Utils {
    class OpenGLUtils {
    public:
        static GLuint GenerateTexture(int width, int height, int channels, const unsigned char* data) {
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
                glGenTextures(1, &textureID);
                if (textureID == 0) {
                    GLenum error = glGetError();
                    std::cerr << "GenerateTexture: glGenTextures failed with error: " << error << std::endl;
                    return 0;
                }

                // Save all unpack state
                GLint oldAlignment, oldRowLength, oldSkipPixels, oldSkipRows, oldImageHeight;
                glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment);
                glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
                glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &oldSkipPixels);
                glGetIntegerv(GL_UNPACK_SKIP_ROWS, &oldSkipRows);
                glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &oldImageHeight);

                // Log state before
                std::cout << "GenerateTexture BEFORE: ALIGN=" << oldAlignment
                    << " ROW_LEN=" << oldRowLength
                    << " SKIP_PIX=" << oldSkipPixels
                    << " SKIP_ROW=" << oldSkipRows
                    << " IMG_H=" << oldImageHeight
                    << " (w=" << width << " h=" << height << " ch=" << channels << ")" << std::endl;

                // Set to safe values for our upload
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                GLenum format, internalFormat;
                switch (channels) {
                case 1: format = GL_RED; internalFormat = GL_RED; break;
                case 3: format = GL_RGB; internalFormat = GL_RGB; break;
                case 4: format = GL_RGBA; internalFormat = GL_RGBA; break;
                default:
                    glDeleteTextures(1, &textureID);
                    return 0;
                }

                glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                    format, GL_UNSIGNED_BYTE, data);

                // Restore original unpack state
                glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, oldSkipPixels);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, oldSkipRows);
                glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, oldImageHeight);

                // Log state after restore
                GLint afterAlign, afterRowLen, afterSkipPix, afterSkipRow, afterImgH;
                glGetIntegerv(GL_UNPACK_ALIGNMENT, &afterAlign);
                glGetIntegerv(GL_UNPACK_ROW_LENGTH, &afterRowLen);
                glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &afterSkipPix);
                glGetIntegerv(GL_UNPACK_SKIP_ROWS, &afterSkipRow);
                glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &afterImgH);
                std::cout << "GenerateTexture AFTER: ALIGN=" << afterAlign
                    << " ROW_LEN=" << afterRowLen
                    << " SKIP_PIX=" << afterSkipPix
                    << " SKIP_ROW=" << afterSkipRow
                    << " IMG_H=" << afterImgH
                    << " (texID=" << textureID << ")" << std::endl;

                GLenum texError = glGetError();
                if (texError != GL_NO_ERROR) {
                    std::cerr << "GenerateTexture: glTexImage2D failed with error: " << texError
                        << " for " << width << "x" << height << ", channels: " << channels << std::endl;
                    glDeleteTextures(1, &textureID);
                    return 0;
                }

                glBindTexture(GL_TEXTURE_2D, 0);
                std::cout << "GenerateTexture: Created texture " << textureID
                    << " (" << width << "x" << height << ", " << channels << "ch)" << std::endl;
                return textureID;
            }
            catch (...) {
                if (textureID) glDeleteTextures(1, &textureID);
                return 0;
            }
        }

        static void DeleteTexture(GLuint& textureID) {
            if (textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
        }

        static bool IsValidTexture(GLuint textureID) {
            return textureID != 0 && glIsTexture(textureID);
        }

        static bool UpdateTexture(GLuint textureID, int width, int height, int channels, const unsigned char* data) {
            if (!IsValidTexture(textureID) || !data) return false;

            GLenum format;
            if (channels == 1) format = GL_RED;
            else if (channels == 3) format = GL_RGB;
            else if (channels == 4) format = GL_RGBA;
            else return false;

            // Save all unpack state
            GLint oldAlignment, oldRowLength, oldSkipPixels, oldSkipRows, oldImageHeight;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment);
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
            glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &oldSkipPixels);
            glGetIntegerv(GL_UNPACK_SKIP_ROWS, &oldSkipRows);
            glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &oldImageHeight);

            // Log state before
            std::cout << "UpdateTexture BEFORE: ALIGN=" << oldAlignment
                << " ROW_LEN=" << oldRowLength
                << " SKIP_PIX=" << oldSkipPixels
                << " SKIP_ROW=" << oldSkipRows
                << " IMG_H=" << oldImageHeight
                << " (texID=" << textureID << " w=" << width << " h=" << height << ")" << std::endl;

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

            // Restore
            glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, oldSkipPixels);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, oldSkipRows);
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, oldImageHeight);

            // Log state after restore
            GLint afterAlign, afterRowLen, afterSkipPix, afterSkipRow, afterImgH;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &afterAlign);
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &afterRowLen);
            glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &afterSkipPix);
            glGetIntegerv(GL_UNPACK_SKIP_ROWS, &afterSkipRow);
            glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &afterImgH);
            std::cout << "UpdateTexture AFTER: ALIGN=" << afterAlign
                << " ROW_LEN=" << afterRowLen
                << " SKIP_PIX=" << afterSkipPix
                << " SKIP_ROW=" << afterSkipRow
                << " IMG_H=" << afterImgH
                << " (texID=" << textureID << ")" << std::endl;

            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                std::cerr << "UpdateTexture: glTexSubImage2D failed with error: " << error << std::endl;
                glBindTexture(GL_TEXTURE_2D, 0);
                return false;
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            return true;
        }

        static GLuint CreateTextureWithParams(int width, int height, GLenum internalFormat, GLenum format,
            GLenum type, const void* data, GLint minFilter = GL_NEAREST,
            GLint magFilter = GL_NEAREST, GLint wrapS = GL_CLAMP_TO_EDGE,
            GLint wrapT = GL_CLAMP_TO_EDGE) {
            GLuint textureID = 0;
            glGenTextures(1, &textureID);
            if (textureID == 0) return 0;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

            // Save all unpack state
            GLint oldAlignment, oldRowLength, oldSkipPixels, oldSkipRows, oldImageHeight;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldAlignment);
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
            glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &oldSkipPixels);
            glGetIntegerv(GL_UNPACK_SKIP_ROWS, &oldSkipRows);
            glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &oldImageHeight);

            // Log state before
            std::cout << "CreateTextureWithParams BEFORE: ALIGN=" << oldAlignment
                << " ROW_LEN=" << oldRowLength
                << " SKIP_PIX=" << oldSkipPixels
                << " SKIP_ROW=" << oldSkipRows
                << " IMG_H=" << oldImageHeight
                << " (w=" << width << " h=" << height << ")" << std::endl;

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);

            // Restore
            glPixelStorei(GL_UNPACK_ALIGNMENT, oldAlignment);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);
            glPixelStorei(GL_UNPACK_SKIP_PIXELS, oldSkipPixels);
            glPixelStorei(GL_UNPACK_SKIP_ROWS, oldSkipRows);
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, oldImageHeight);

            // Log state after restore
            GLint afterAlign, afterRowLen, afterSkipPix, afterSkipRow, afterImgH;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &afterAlign);
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &afterRowLen);
            glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &afterSkipPix);
            glGetIntegerv(GL_UNPACK_SKIP_ROWS, &afterSkipRow);
            glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &afterImgH);
            std::cout << "CreateTextureWithParams AFTER: ALIGN=" << afterAlign
                << " ROW_LEN=" << afterRowLen
                << " SKIP_PIX=" << afterSkipPix
                << " SKIP_ROW=" << afterSkipRow
                << " IMG_H=" << afterImgH
                << " (texID=" << textureID << ")" << std::endl;

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