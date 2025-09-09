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

#include "PngMetadataUtils.hpp"
#include "ImageUtils.hpp"
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

		std::string outputPath = fullPath;
		if (outputPath.empty())
		{
			outputPath = Utils::FilePaths::defaultProjectPath + "/AniStudio_output.png";
			std::cout << "Empty output path, using default: " << outputPath << std::endl;
		}

		// Ensure the path is absolute and valid
		std::filesystem::path outputFilePath(outputPath);

		// If path is relative, make it relative to default project path
		if (outputFilePath.is_relative())
		{
			outputFilePath = std::filesystem::path(Utils::FilePaths::defaultProjectPath) / outputFilePath;
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