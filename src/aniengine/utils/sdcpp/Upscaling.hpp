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

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "pch.h"
#include <stb_image.h>
#include <stb_image_write.h>

namespace Utils
{
	// Forward declarations for shared utilities
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);

	class Upscaling
	{
	public:
		static bool RunUpscaling(const nlohmann::json &metadata, std::string fullPath)
		{
			upscaler_ctx_t *upscaler_context = nullptr;
			unsigned char *inputData = nullptr;

			try
			{
				// Extract parameters from metadata
				std::string inputImagePath = "";
				std::string modelPath = Utils::FilePaths::upscaleDir;
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "upscale_AniStudio.png";
				uint32_t upscaleFactor = 4;
				bool preserveAspectRatio = true;
				int n_threads = 4;

				// Debug logging for metadata
				std::cout << "Upscaling metadata:" << std::endl;
				std::cout << metadata.dump(2) << std::endl;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						// Input image path
						if (comp.contains("InputImage"))
						{
							nlohmann::json inputImageData = comp["InputImage"];

							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null() && !inputImageData["filePath"].get<std::string>().empty())
							{
								inputImagePath = inputImageData["filePath"].get<std::string>();
								std::cout << "Found input path: " << inputImagePath << std::endl;
							}
						}

						// Output settings
						if (comp.contains("OutputImage"))
						{
							nlohmann::json outputImageData = comp["OutputImage"];

							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null() && !outputImageData["filePath"].get<std::string>().empty())
							{
								outputPath = outputImageData["filePath"].get<std::string>();
								std::cout << "Found output path: " << outputPath << std::endl;
							}

							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null() && !outputImageData["fileName"].get<std::string>().empty())
							{
								outputFilename = outputImageData["fileName"].get<std::string>();
								std::cout << "Found output filename: " << outputFilename << std::endl;
							}
						}

						// Esrgan component
						if (comp.contains("Esrgan"))
						{
							nlohmann::json esrganData = comp["Esrgan"];

							if (esrganData.contains("modelPath") && !esrganData["modelPath"].is_null() && !esrganData["modelPath"].get<std::string>().empty())
							{
								modelPath = esrganData["modelPath"];
							}
							else if (esrganData.contains("modelName") && !esrganData["modelName"].is_null() && !esrganData["modelName"].get<std::string>().empty())
							{
								std::string modelName = esrganData["modelName"].get<std::string>();
								modelPath = (std::filesystem::path(Utils::FilePaths::upscaleDir) / modelName).string();
							}

							if (esrganData.contains("upscaleFactor"))
								upscaleFactor = esrganData["upscaleFactor"];

							if (esrganData.contains("preserveAspectRatio"))
								preserveAspectRatio = esrganData["preserveAspectRatio"];
						}

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("n_threads"))
								n_threads = samplerData["n_threads"];
						}
					}
				}

				// Validate parameters
				if (inputImagePath.empty())
				{
					throw std::runtime_error("Input image path is empty!");
				}

				if (modelPath.empty())
				{
					throw std::runtime_error("ESRGAN model path is empty!");
				}

				// Load input image
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 0);
				if (!inputData)
				{
					std::string error = std::string("Failed to load input image: ") + inputImagePath + " - " +
						(stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
					throw std::runtime_error(error);
				}

				std::cout << "Input image loaded successfully: " << inputWidth << "x" << inputHeight
					<< " with " << inputChannels << " channels" << std::endl;

				// Create output path
				std::filesystem::path outputDir(outputPath);
				std::filesystem::path outputFile(outputFilename);
				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputFile.string(), outputDir.string());

				// Initialize upscaler context
				upscaler_context = new_upscaler_ctx(modelPath.c_str(), n_threads);
				if (!upscaler_context)
				{
					throw std::runtime_error("Failed to initialize upscaler context!");
				}

				// Create input image struct
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					static_cast<uint32_t>(inputChannels),
					inputData };

				// Perform upscaling
				sd_image_t upscaled_image = upscale(upscaler_context, input_image, upscaleFactor);
				if (!upscaled_image.data)
				{
					throw std::runtime_error("Upscaling failed - no output image produced");
				}

				std::cout << "Upscaling successful, saving output to: " << uniqueFilePath << std::endl;

				// Update metadata with correct output path before saving
				nlohmann::json updatedMetadata = metadata;
				for (auto &comp : updatedMetadata["components"])
				{
					if (comp.contains("OutputImage"))
					{
						if (comp["OutputImage"].contains("OutputImage"))
						{
							comp["OutputImage"]["OutputImage"]["filePath"] = uniqueFilePath;
						}
						else
						{
							comp["OutputImage"]["filePath"] = uniqueFilePath;
						}
					}
				}

				// Save the upscaled image
				SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
					upscaled_image.channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				// Free the upscaled image if needed
				if (upscaled_image.data)
				{
					free(upscaled_image.data);
				}

				// Cleanup upscaler context
				free_upscaler_ctx(upscaler_context);
				upscaler_context = nullptr;

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during upscaling: " << e.what() << std::endl;

				// Clean up resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (upscaler_context)
				{
					free_upscaler_ctx(upscaler_context);
					upscaler_context = nullptr;
				}

				return false;
			}
		}
	};
}