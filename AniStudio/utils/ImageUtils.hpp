#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <png.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <nlohmann/json.hpp>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>
#include <GL/glew.h>

namespace Utils {

    class ImageUtils {
    public:
        // Generate a unique filename with incremental numbering
        static std::string GenerateNumberedFilename(const std::string& baseName,
            const std::string& extension,
            const std::string& directory) {
            // Ensure directory exists
            std::filesystem::path dirPath(directory);
            if (!std::filesystem::exists(dirPath)) {
                std::filesystem::create_directories(dirPath);
            }

            // Find the highest existing index
            int highestIndex = 0;
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (entry.path().extension() == extension) {
                    std::string filename = entry.path().stem().string();
                    size_t lastDashPos = filename.find_last_of('-');
                    if (lastDashPos != std::string::npos &&
                        filename.substr(0, lastDashPos) == baseName) {
                        try {
                            int index = std::stoi(filename.substr(lastDashPos + 1));
                            highestIndex = std::max(highestIndex, index);
                        }
                        catch (const std::invalid_argument&) {
                            // Skip files that don't match the pattern
                        }
                    }
                }
            }

            // Format the new filename
            std::ostringstream formattedIndex;
            formattedIndex << std::setw(5) << std::setfill('0') << (highestIndex + 1);
            return baseName + "-" + formattedIndex.str() + extension;
        }

        // Generate a timestamp-based unique filename
        static std::string GenerateTimestampFilename(const std::string& prefix,
            const std::string& extension) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::ostringstream ss;
            ss << prefix << "_";

            // Format the timestamp
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time);
#else
            localtime_r(&time, &tm);
#endif

            ss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << ms.count();
            ss << extension;

            return ss.str();
        }

        // Save image data to a PNG file
        static bool SaveImageToPNG(const unsigned char* data,
            int width, int height, int channels,
            const std::string& filePath) {
            if (!data || width <= 0 || height <= 0 || channels <= 0) {
                std::cerr << "Invalid image data for saving" << std::endl;
                return false;
            }

            // Make sure the directory exists
            std::filesystem::path path(filePath);
            try {
                std::filesystem::create_directories(path.parent_path());
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory: " << e.what() << std::endl;
                return false;
            }

            // Save the image
            int stride = width * channels;
            bool success = stbi_write_png(filePath.c_str(), width, height, channels, data, stride);

            if (!success) {
                std::cerr << "Failed to save image to: " << filePath << std::endl;
            }

            return success;
        }

        // Write metadata to a PNG file
        static bool WritePNGMetadata(const std::string& filePath,
            const nlohmann::json& metadata) {
            // Open the PNG file for reading
            FILE* fp = fopen(filePath.c_str(), "rb");
            if (!fp) {
                std::cerr << "Failed to open PNG for reading: " << filePath << std::endl;
                return false;
            }

            // Verify PNG signature
            unsigned char header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                std::cerr << "Not a valid PNG file" << std::endl;
                fclose(fp);
                return false;
            }
            fseek(fp, 0, SEEK_SET);

            // Initialize PNG read structures
            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                fclose(fp);
                return false;
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            // Get image info
            png_uint_32 width, height;
            int bit_depth, color_type;
            png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type,
                nullptr, nullptr, nullptr);

            // Create temporary file
            std::string tempFile = filePath + ".tmp";
            FILE* out = fopen(tempFile.c_str(), "wb");
            if (!out) {
                std::cerr << "Failed to create temporary file" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            // Initialize PNG write structures
            png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!pngWrite) {
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite) {
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite))) {
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(pngWrite, out);

            // Copy IHDR
            png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);

            // Convert metadata to string
            std::string metadataStr = metadata.dump();

            // Set up metadata chunks
            std::vector<png_text> texts;

            // Parameters text chunk
            png_text paramText;
            paramText.compression = PNG_TEXT_COMPRESSION_NONE;
            paramText.key = const_cast<char*>("parameters");
            paramText.text = const_cast<char*>(metadataStr.c_str());
            paramText.text_length = metadataStr.length();
            paramText.itxt_length = 0;
            paramText.lang = nullptr;
            paramText.lang_key = nullptr;
            texts.push_back(paramText);

            // Software identifier
            png_text softwareText;
            softwareText.compression = PNG_TEXT_COMPRESSION_NONE;
            softwareText.key = const_cast<char*>("Software");
            softwareText.text = const_cast<char*>("AniStudio");
            softwareText.text_length = 9;
            softwareText.itxt_length = 0;
            softwareText.lang = nullptr;
            softwareText.lang_key = nullptr;
            texts.push_back(softwareText);

            // Write the text chunks
            png_set_text(pngWrite, infoWrite, texts.data(), texts.size());

            // Write PNG header
            png_write_info(pngWrite, infoWrite);

            // Copy image data
            std::vector<png_byte> row(png_get_rowbytes(png, info));
            for (png_uint_32 y = 0; y < height; y++) {
                png_read_row(png, row.data(), nullptr);
                png_write_row(pngWrite, row.data());
            }

            // Finish writing
            png_write_end(pngWrite, infoWrite);

            // Clean up
            png_destroy_write_struct(&pngWrite, &infoWrite);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(out);
            fclose(fp);

            // Replace original with new file
            try {
                std::filesystem::path originalPath(filePath);
                std::filesystem::path tempPath(tempFile);

                if (std::filesystem::exists(originalPath)) {
                    std::filesystem::remove(originalPath);
                }

                std::filesystem::rename(tempPath, originalPath);
                return true;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }

        // Read metadata from a PNG file
        static nlohmann::json ReadPNGMetadata(const std::string& filePath) {
            nlohmann::json result;

            FILE* fp = fopen(filePath.c_str(), "rb");
            if (!fp) {
                return result;
            }

            // Verify PNG signature
            unsigned char header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
                fclose(fp);
                return result;
            }
            fseek(fp, 0, SEEK_SET);

            // Initialize PNG read structures
            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png) {
                fclose(fp);
                return result;
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return result;
            }

            if (setjmp(png_jmpbuf(png))) {
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return result;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            // Read text chunks
            png_textp text_ptr;
            int num_text;
            if (png_get_text(png, info, &text_ptr, &num_text) > 0) {
                for (int i = 0; i < num_text; i++) {
                    if (strcmp(text_ptr[i].key, "parameters") == 0) {
                        try {
                            result = nlohmann::json::parse(text_ptr[i].text);
                            break;
                        }
                        catch (const nlohmann::json::parse_error&) {
                            // Parse error, return empty JSON
                        }
                    }
                }
            }

            // Clean up
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);

            return result;
        }

        // Extract a meaningful base name from a prompt
        static std::string ExtractBaseNameFromPrompt(const std::string& prompt,
            size_t maxLength = 20) {
            if (prompt.empty()) {
                return "generated";
            }

            std::string result;
            size_t wordCount = 0;
            size_t pos = 0;

            // Skip initial spaces
            while (pos < prompt.length() && isspace(prompt[pos])) pos++;

            // Extract up to 3 words or until we hit the maxLength
            while (pos < prompt.length() && wordCount < 3 && result.length() < maxLength) {
                // Find next space
                size_t wordEnd = prompt.find_first_of(" \t\n", pos);
                if (wordEnd == std::string::npos) wordEnd = prompt.length();

                // Extract word
                std::string word = prompt.substr(pos, wordEnd - pos);

                // Add word if not empty and fits within limits
                if (!word.empty()) {
                    if (!result.empty()) result += "_";

                    // Truncate word if needed to stay within limits
                    if (result.length() + word.length() > maxLength) {
                        word = word.substr(0, maxLength - result.length());
                    }

                    result += word;
                    wordCount++;
                }

                // Move to next word
                pos = wordEnd;
                while (pos < prompt.length() && isspace(prompt[pos])) pos++;
            }

            // If we didn't get any usable text, return default
            if (result.empty()) {
                return "generated";
            }

            // Replace invalid characters
            for (char& c : result) {
                if (!isalnum(c) && c != '_' && c != '-') {
                    c = '_';
                }
            }

            return result;
        }

        // Load image data from a file
        static unsigned char* LoadImageData(const std::string& filePath, int* width, int* height, int* channels) {
            if (!width || !height || !channels) {
                std::cerr << "Invalid output parameters for LoadImageData" << std::endl;
                return nullptr;
            }

            // Configure stb_image for OpenGL
            stbi_set_flip_vertically_on_load(true);

            // Load the image
            unsigned char* data = stbi_load(filePath.c_str(), width, height, channels, 0);

            if (!data) {
                std::cerr << "Failed to load image: " << filePath << " - " << stbi_failure_reason() << std::endl;
            }

            return data;
        }

        // Free image data
        static void FreeImageData(unsigned char* data) {
            if (data) {
                stbi_image_free(data);
            }
        }

        // Copy image data to a new buffer
        static unsigned char* CopyImageData(const unsigned char* data, int width, int height, int channels) {
            if (!data || width <= 0 || height <= 0 || channels <= 0) {
                return nullptr;
            }

            size_t dataSize = width * height * channels;
            unsigned char* newData = static_cast<unsigned char*>(malloc(dataSize));

            if (newData) {
                memcpy(newData, data, dataSize);
            }

            return newData;
        }

        // Resize an image to new dimensions (simple nearest neighbor)
        static unsigned char* ResizeImage(const unsigned char* data, int width, int height, int channels,
            int newWidth, int newHeight) {
            if (!data || width <= 0 || height <= 0 || channels <= 0 ||
                newWidth <= 0 || newHeight <= 0) {
                return nullptr;
            }

            // Allocate memory for the resized image
            unsigned char* resizedData = static_cast<unsigned char*>(malloc(newWidth * newHeight * channels));
            if (!resizedData) {
                return nullptr;
            }

            // Simple nearest-neighbor scaling
            float xRatio = static_cast<float>(width) / static_cast<float>(newWidth);
            float yRatio = static_cast<float>(height) / static_cast<float>(newHeight);

            for (int y = 0; y < newHeight; y++) {
                int srcY = static_cast<int>(y * yRatio);
                for (int x = 0; x < newWidth; x++) {
                    int srcX = static_cast<int>(x * xRatio);
                    for (int c = 0; c < channels; c++) {
                        resizedData[(y * newWidth + x) * channels + c] =
                            data[(srcY * width + srcX) * channels + c];
                    }
                }
            }

            return resizedData;
        }
    };

} // namespace Utils