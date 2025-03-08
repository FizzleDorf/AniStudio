#pragma once

#include <string>
#include <GL/glew.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace Utils {

    class ImageUtils {
    public:
        // Load image data from a file (does not create OpenGL texture)
        static unsigned char* LoadImageData(const std::string& filePath, int& width, int& height, int& channels) {
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                std::cerr << "Failed to load image: " << filePath << " - " << stbi_failure_reason() << std::endl;
                return nullptr;
            }
            return data;
        }

        // Free image data
        static void FreeImageData(unsigned char* data) {
            if (data) {
                stbi_image_free(data);
            }
        }

        // Create a copy of image data
        static unsigned char* CopyImageData(const unsigned char* src, int width, int height, int channels) {
            if (!src) return nullptr;

            size_t dataSize = width * height * channels;
            unsigned char* copy = static_cast<unsigned char*>(malloc(dataSize));

            if (copy) {
                memcpy(copy, src, dataSize);
            }

            return copy;
        }

        // Save an image to a file
        static bool SaveImage(const std::string& filePath, int width, int height, int channels, const unsigned char* data) {
            // Create the directory if it doesn't exist
            std::filesystem::path path(filePath);
            std::filesystem::create_directories(path.parent_path());

            // Determine file format based on extension
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            int result = 0;

            if (ext == ".png") {
                result = stbi_write_png(filePath.c_str(), width, height, channels, data, width * channels);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                result = stbi_write_jpg(filePath.c_str(), width, height, channels, data, 90); // 90% quality
            }
            else if (ext == ".bmp") {
                result = stbi_write_bmp(filePath.c_str(), width, height, channels, data);
            }
            else if (ext == ".tga") {
                result = stbi_write_tga(filePath.c_str(), width, height, channels, data);
            }
            else {
                // Default to PNG if extension is not recognized
                std::string newPath = filePath + ".png";
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
            }

            return result != 0;
        }

        // Generate a unique filename using an incremental counter
        static std::string CreateUniqueFilenameIncremental(const std::string& baseName, const std::string& directory, const std::string& extension) {
            std::filesystem::path dirPath(directory);
            std::filesystem::path basePath(baseName);

            // Create the directory if it doesn't exist
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
            }

            std::string filename = basePath.stem().string();
            std::string normalizedExt = extension;

            // Ensure extension starts with a dot
            if (!extension.empty() && extension[0] != '.') {
                normalizedExt = "." + extension;
            }

            // Find the highest existing index in the directory
            int highestIndex = 0;
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (entry.path().extension() == normalizedExt) {
                    std::string entryName = entry.path().stem().string();
                    if (entryName.find(filename + "_") == 0) {
                        try {
                            size_t underscorePos = entryName.find('_');
                            if (underscorePos != std::string::npos) {
                                int index = std::stoi(entryName.substr(underscorePos + 1));
                                if (index > highestIndex) {
                                    highestIndex = index;
                                }
                            }
                        }
                        catch (const std::exception&) {
                            // Ignore conversion errors
                        }
                    }
                }
            }

            // Create new filename with incremented index
            std::string newFilename = filename + "_" + std::to_string(highestIndex + 1) + normalizedExt;
            return (dirPath / newFilename).string();
        }

        // Generate a unique filename using the current datetime
        static std::string CreateUniqueFilenameDatetime(const std::string& baseName, const std::string& directory, const std::string& extension) {
            std::filesystem::path dirPath(directory);
            std::filesystem::path basePath(baseName);

            // Create the directory if it doesn't exist
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
            }

            // Get current time
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);

            // Format the time as YYYYMMDD_HHMMSS
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");

            // Ensure extension starts with a dot
            std::string normalizedExt = extension;
            if (!extension.empty() && extension[0] != '.') {
                normalizedExt = "." + extension;
            }

            // Create the filename with the datetime suffix
            std::string filename = basePath.stem().string() + "_" + ss.str() + normalizedExt;
            return (dirPath / filename).string();
        }

        // Utility function to generate an OpenGL texture from image data
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

        // Delete an OpenGL texture
        static void DeleteTexture(GLuint& textureID) {
            if (textureID != 0) {
                glDeleteTextures(1, &textureID);
                textureID = 0;
            }
        }
    };

} // namespace Utils