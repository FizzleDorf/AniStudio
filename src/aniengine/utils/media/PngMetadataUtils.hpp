#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <png.h>
#include <nlohmann/json.hpp>
#include "MetadataUtils.hpp"
#include <memory>

namespace Utils
{
    class PngMetadata
    {
    public:
        static bool WriteMetadataToPNG(const std::string& imagePath, const nlohmann::json& metadata,
            const std::string& softwareTag = "AniStudio")
        {
            FILE* fp = fopen(imagePath.c_str(), "rb");
            if (!fp)
            {
                std::cerr << "Failed to open PNG for reading: " << imagePath << std::endl;
                return false;
            }

            unsigned char header[8];
            if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8))
            {
                std::cerr << "Not a valid PNG file" << std::endl;
                fclose(fp);
                return false;
            }
            fseek(fp, 0, SEEK_SET);

            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!png)
            {
                std::cerr << "Failed to create PNG read struct" << std::endl;
                fclose(fp);
                return false;
            }

            png_infop info = png_create_info_struct(png);
            if (!info)
            {
                std::cerr << "Failed to create PNG info struct" << std::endl;
                png_destroy_read_struct(&png, nullptr, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(png)))
            {
                std::cerr << "Error during PNG read initialization" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(png, fp);
            png_read_info(png, info);

            png_uint_32 width, height;
            int bit_depth, color_type;
            png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

            std::string tempFile = imagePath + ".tmp";
            FILE* out = fopen(tempFile.c_str(), "wb");
            if (!out)
            {
                std::cerr << "Failed to create temporary file" << std::endl;
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
            if (!pngWrite)
            {
                std::cerr << "Failed to create PNG write struct" << std::endl;
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_infop infoWrite = png_create_info_struct(pngWrite);
            if (!infoWrite)
            {
                std::cerr << "Failed to create PNG write info struct" << std::endl;
                png_destroy_write_struct(&pngWrite, nullptr);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            if (setjmp(png_jmpbuf(pngWrite)))
            {
                std::cerr << "Error during PNG write initialization" << std::endl;
                png_destroy_write_struct(&pngWrite, &infoWrite);
                fclose(out);
                png_destroy_read_struct(&png, &info, nullptr);
                fclose(fp);
                return false;
            }

            png_init_io(pngWrite, out);

            png_set_IHDR(pngWrite, infoWrite, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            std::string metadataStr = metadata.dump();
            std::vector<png_text> texts;

            png_text paramText;
            paramText.compression = PNG_TEXT_COMPRESSION_NONE;
            paramText.key = const_cast<char*>("parameters");
            paramText.text = const_cast<char*>(metadataStr.c_str());
            paramText.text_length = metadataStr.length();
            paramText.itxt_length = 0;
            paramText.lang = nullptr;
            paramText.lang_key = nullptr;
            texts.push_back(paramText);

            png_text softwareText;
            softwareText.compression = PNG_TEXT_COMPRESSION_NONE;
            softwareText.key = const_cast<char*>("Software");
            softwareText.text = const_cast<char*>(softwareTag.c_str());
            softwareText.text_length = softwareTag.length();
            softwareText.itxt_length = 0;
            softwareText.lang = nullptr;
            softwareText.lang_key = nullptr;
            texts.push_back(softwareText);

            png_set_text(pngWrite, infoWrite, texts.data(), static_cast<int>(texts.size()));
            png_write_info(pngWrite, infoWrite);

            std::vector<png_byte> row(png_get_rowbytes(png, info));
            for (png_uint_32 y = 0; y < height; y++)
            {
                png_read_row(png, row.data(), nullptr);
                png_write_row(pngWrite, row.data());
            }

            png_write_end(pngWrite, infoWrite);

            png_destroy_write_struct(&pngWrite, &infoWrite);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(out);
            fclose(fp);

            try
            {
                std::filesystem::path originalPath(imagePath);
                std::filesystem::path tempPath(tempFile);

                if (std::filesystem::exists(originalPath))
                {
                    std::filesystem::remove(originalPath);
                }

                std::filesystem::rename(tempPath, originalPath);
                std::cout << "Successfully wrote metadata to PNG: " << imagePath << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "Error replacing file: " << e.what() << std::endl;
                return false;
            }
        }

        static std::string CreateUniqueFilename(const std::string& baseFilename, const std::string& directory)
        {
            if (directory.empty())
            {
                std::cerr << "[PngMetadata] Directory is empty, cannot create unique filename" << std::endl;
                return baseFilename;
            }

            try
            {
                std::string validBaseName = baseFilename.empty() ? "AniStudio_output.png" : baseFilename;
                std::filesystem::path directoryPath(directory);

                if (!directoryPath.is_absolute())
                {
                    directoryPath = std::filesystem::current_path() / directoryPath;
                }

                std::error_code ec;
                std::filesystem::create_directories(directoryPath, ec);
                if (ec)
                {
                    std::cerr << "Failed to create directory: " << directoryPath.string() << " - " << ec.message() << std::endl;
                    directoryPath = std::filesystem::current_path();
                    std::filesystem::create_directories(directoryPath, ec);
                    if (ec)
                    {
                        throw std::runtime_error("Failed to create fallback directory: " + directoryPath.string());
                    }
                }

                std::filesystem::path originalFilePath(validBaseName);
                std::string baseName = originalFilePath.stem().string();
                std::string extension = originalFilePath.extension().string();

                if (!extension.empty() && extension[0] != '.')
                {
                    extension = "." + extension;
                }

                if (extension.empty())
                {
                    extension = ".png";
                }

                if (!std::filesystem::exists(directoryPath))
                {
                    throw std::runtime_error("Directory does not exist after creation: " + directoryPath.string());
                }

                int highestIndex = 0;
                for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
                {
                    if (entry.is_regular_file())
                    {
                        std::string entryName = entry.path().stem().string();
                        std::string entryExt = entry.path().extension().string();

                        if (entryExt == extension)
                        {
                            std::string prefix = baseName + "-";
                            if (entryName.find(prefix) == 0)
                            {
                                std::string numberPart = entryName.substr(prefix.length());
                                try
                                {
                                    int index = std::stoi(numberPart);
                                    if (index > highestIndex)
                                    {
                                        highestIndex = index;
                                    }
                                }
                                catch (const std::exception&)
                                {
                                    continue;
                                }
                            }
                        }
                    }
                }

                highestIndex++;
                std::ostringstream formattedIndex;
                formattedIndex << std::setw(5) << std::setfill('0') << highestIndex;

                std::filesystem::path newFilePath = directoryPath / (baseName + "-" + formattedIndex.str() + extension);
                return newFilePath.string();
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error in CreateUniqueFilename: " << e.what() << std::endl;
                return "AniStudio_fallback.png";
            }
        }

        static nlohmann::json ReadMetadataFromPNG(const std::string& imagePath)
        {
            return MetadataUtils::LoadMetadataFromPNG(imagePath);
        }

        static nlohmann::json CreateGenerationMetadata(const nlohmann::json& entityData,
            const nlohmann::json& additionalInfo = {})
        {
            return MetadataUtils::CreateGenerationMetadata(entityData, additionalInfo);
        }
    };

} // namespace Utils