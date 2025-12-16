#pragma once

#include "PngMetadataUtils.hpp"
#include "ImageUtils.hpp"
#include "FilePaths.hpp"  // Added include for new FilePaths
#include "pch.h"
#include <filesystem>
#include <iostream>

namespace Utils {

	// Common image saving functionality
	inline void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath)
	{
		try
		{
			// Validate input parameters
			if (!data)
			{
				throw std::runtime_error("Image data is null");
			}

			if (width <= 0 || height <= 0)
			{
				throw std::runtime_error("Invalid image dimensions: " + std::to_string(width) + "x" + std::to_string(height));
			}

			// Get FilePaths instance
			FilePaths& filePaths = FilePaths::GetInstance();

			std::string outputPath = fullPath;
			if (outputPath.empty())
			{
				// Get default project path from FilePaths
				std::string defaultProjectPath = filePaths.GetPath("DefaultProject");

				if (defaultProjectPath.empty() || defaultProjectPath[0] == '\0') {
					// Fallback to executable directory
					defaultProjectPath = filePaths.GetExecutableDir();
				}

				outputPath = defaultProjectPath + "/AniStudio_output.png";
				std::cout << "Empty output path, using default: " << outputPath << std::endl;
			}

			// Ensure the path is absolute and valid
			std::filesystem::path outputFilePath(outputPath);

			// If path is relative, make it relative to default project path
			if (outputFilePath.is_relative())
			{
				// Get default project path from FilePaths
				std::string defaultProjectPath = filePaths.GetPath("DefaultProject");

				if (defaultProjectPath.empty() || defaultProjectPath[0] == '\0') {
					// Fallback to executable directory
					defaultProjectPath = filePaths.GetExecutableDir();
				}

				outputFilePath = std::filesystem::path(defaultProjectPath) / outputFilePath;
				outputPath = outputFilePath.string();
			}

			// Ensure parent directory exists
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

			// Validate that the directory is writable
			if (!std::filesystem::exists(parentDir))
			{
				throw std::runtime_error("Directory does not exist after creation: " + parentDir.string());
			}

			// Save file
			if (!Utils::ImageUtils::SaveImage(outputPath, width, height, channels, data))
			{
				throw std::runtime_error("Failed to save image to: " + outputPath);
			}

			// Save metadata to the PNG (only if file was saved successfully)
			try
			{
				Utils::PngMetadata::WriteMetadataToPNG(outputPath, metadata);
			}
			catch (const std::exception &e)
			{
				std::cerr << "Warning: Failed to write metadata to PNG: " << e.what() << std::endl;
				// Don't throw here, image was saved successfully
			}

			std::cout << "Image saved successfully: " << outputPath << std::endl;
		}
		catch (const std::filesystem::filesystem_error &e)
		{
			std::cerr << "Filesystem error in SaveImage: " << e.what() << std::endl;
			throw;
		}
		catch (const std::exception &e)
		{
			std::cerr << "Exception in SaveImage: " << e.what() << std::endl;
			throw;
		}
	}

} // namespace Utils