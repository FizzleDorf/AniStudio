#pragma once

#include "PngMetadataUtils.hpp"
#include "ImageUtils.hpp"
#include "pch.h"
#include <filesystem>
#include <iostream>

namespace ECS { class FilePathSystem; }
namespace Utils { extern ECS::FilePathSystem* g_FilePathSystem; }

namespace Utils {

    inline void SaveImage(const unsigned char* data, int width, int height, int channels,
        const nlohmann::json& metadata, const std::string& fullPath)
    {
        try
        {
            if (!data)
            {
                throw std::runtime_error("Image data is null");
            }

            if (width <= 0 || height <= 0)
            {
                throw std::runtime_error("Invalid image dimensions: " + std::to_string(width) + "x" + std::to_string(height));
            }

            std::string outputPath = fullPath;
            if (outputPath.empty())
            {
                std::string defaultProjectPath;
                if (g_FilePathSystem) {
                    defaultProjectPath = g_FilePathSystem->GetPath("DefaultProject");
                }
                if (defaultProjectPath.empty()) {
                    defaultProjectPath = ".";
                }
                outputPath = defaultProjectPath + "/AniStudio_output.png";
                std::cout << "Empty output path, using default: " << outputPath << std::endl;
            }

            std::filesystem::path outputFilePath(outputPath);

            if (outputFilePath.is_relative())
            {
                std::string defaultProjectPath;
                if (g_FilePathSystem) {
                    defaultProjectPath = g_FilePathSystem->GetPath("DefaultProject");
                }
                if (defaultProjectPath.empty()) {
                    defaultProjectPath = ".";
                }
                outputFilePath = std::filesystem::path(defaultProjectPath) / outputFilePath;
                outputPath = outputFilePath.string();
            }

            std::filesystem::path parentDir = outputFilePath.parent_path();
            if (!parentDir.empty())
            {
                std::error_code ec;
                std::filesystem::create_directories(parentDir, ec);
                if (ec)
                {
                    throw std::runtime_error("Failed to create directory: " + parentDir.string() + " - " + ec.message());
                }
            }

            if (!std::filesystem::exists(parentDir))
            {
                throw std::runtime_error("Directory does not exist after creation: " + parentDir.string());
            }

            if (!Utils::ImageUtils::SaveImage(outputPath, width, height, channels, data))
            {
                throw std::runtime_error("Failed to save image to: " + outputPath);
            }

            try
            {
                Utils::PngMetadata::WriteMetadataToPNG(outputPath, metadata);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Warning: Failed to write metadata to PNG: " << e.what() << std::endl;
            }

            std::cout << "Image saved successfully: " << outputPath << std::endl;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Filesystem error in SaveImage: " << e.what() << std::endl;
            throw;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in SaveImage: " << e.what() << std::endl;
            throw;
        }
    }

}