#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <png.h>
#include <nlohmann/json.hpp>
#include "MetadataUtils.hpp"

namespace Utils {

    class PngMetadata {
    public:

        static bool WriteMetadataToPNG(const std::string& imagePath, const nlohmann::json& metadata,
            const std::string& softwareTag = "AniStudio") {
            // Open the PNG file for reading
            FILE* fp = fopen(imagePath.c_str(), "rb");
            if (!fp) {
                std::cerr << "Failed to open PNG for reading: " << imagePath << std::endl;
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
                std::cerr << "Failed to create PNG read struct" << std::endl;
                fclose(fp);
                return false;
            }

            png_infop info = png_create_info_struct(png);
            if (!info) {
                std::cerr << "Failed to create PNG info struct" << std::endl;
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(png))) {
                std::cerr << "Error during PNG read initialization" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            // Get image info
            png_uint_32 width, height;
            int bit_depth, color_type;
            png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

            // Create temporary file
            std::string tempFile = imagePath + ".tmp";
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
                std::cerr << "Failed to create PNG write struct" << std::endl;
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite) {
                std::cerr << "Failed to create PNG write info struct" << std::endl;
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite))) {
                std::cerr << "Error during PNG write initialization" << std::endl;
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(pngWrite, out);

            // Copy IHDR
            png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            // Set up metadata chunks
            std::string metadataStr = metadata.dump();
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
            softwareText.text = const_cast<char*>(softwareTag.c_str());
            softwareText.text_length = softwareTag.length();
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
                std::filesystem::path originalPath(imagePath);
                std::filesystem::path tempPath(tempFile);

                // Remove original file
                if (std::filesystem::exists(originalPath)) {
                    std::filesystem::remove(originalPath);
                }

                // Rename temp file to original
                std::filesystem::rename(tempPath, originalPath);
                std::cout << "Successfully wrote metadata to PNG: " << imagePath << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }

        static nlohmann::json ReadMetadataFromPNG(const std::string& imagePath) {
            return MetadataUtils::LoadMetadataFromPNG(imagePath);
        }

		static std::string CreateUniqueFilename(const std::string& baseFilename,
			const std::string& directory) {
			// Create full paths
			std::filesystem::path directoryPath(directory);
			std::filesystem::path originalFilePath(baseFilename);

			// Extract the base name and extension
			std::string baseName = originalFilePath.stem().string();
			std::string extension = originalFilePath.extension().string();

			// Ensure the extension starts with a dot
			if (!extension.empty() && extension[0] != '.') {
				extension = "." + extension;
			}

			// Default to .png if no extension provided
			if (extension.empty()) {
				extension = ".png";
			}

			// Ensure the directory exists
			if (!std::filesystem::exists(directoryPath)) {
				std::filesystem::create_directories(directoryPath);
			}

			// Find the highest existing index
			int highestIndex = 0;
			for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
				if (entry.path().extension().string() == extension) {
					std::string filename = entry.path().stem().string();

					if (filename.find(baseName) == 0) {  // Starts with base name
						size_t dashPos = filename.find_last_of('-');
						if (dashPos != std::string::npos) {
							try {
								int index = std::stoi(filename.substr(dashPos + 1));
								if (index > highestIndex) {
									highestIndex = index;
								}
							}
							catch (...) {
								// Ignore conversion errors
							}
						}
					}
				}
			}

			// Create new filename with incremented index
			highestIndex++;
			std::ostringstream formattedIndex;
			formattedIndex << std::setw(5) << std::setfill('0') << highestIndex;

			// Construct the final path and return it as string
			std::filesystem::path newFilePath = directoryPath / (baseName + "-" + formattedIndex.str() + extension);
			return newFilePath.string();
		}

        static nlohmann::json CreateGenerationMetadata(const nlohmann::json& entityData,
            const nlohmann::json& additionalInfo = {}) {
            return MetadataUtils::CreateGenerationMetadata(entityData, additionalInfo);
        }
    };

} // namespace Utils