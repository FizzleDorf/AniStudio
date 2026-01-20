#pragma once

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "FilePathService.hpp"  // Updated to use FilePathService
#include "pch.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <filesystem>
#include <thread>

namespace Utils
{
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);

	class Upscaling
	{
	public:
		// Note: upscaling doesn't use sd_ctx_t, but we keep the parameter for consistency
		// The sd_context parameter is ignored for upscaling operations
		static bool RunUpscaling(const nlohmann::json &metadata, const std::string &fullPath, sd_ctx_t *sd_context = nullptr)
		{
			// Note: sd_context parameter is not used for upscaling, but kept for API consistency
			(void)sd_context; // Mark as unused to avoid compiler warnings

			upscaler_ctx_t *upscaler_context = nullptr;
			unsigned char *inputData = nullptr;
			sd_image_t upscaled_image = { 0, 0, 0, nullptr };

			try
			{
				// Extract parameters from metadata
				std::string inputImagePath = "";
				std::string modelPath = Utils::FilePathService::GetPath("Upscale");
				std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
				std::string outputFilename = "upscale_AniStudio.png";
				uint32_t upscaleFactor = 4;
				bool offload_params_to_cpu = false;
				bool direct = false;
				int n_threads = 4;
				int tile_size = 64;

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
								modelPath = esrganData["modelPath"].get<std::string>();
							}
							else if (esrganData.contains("modelName") && !esrganData["modelName"].is_null() && !esrganData["modelName"].get<std::string>().empty())
							{
								std::string modelName = esrganData["modelName"].get<std::string>();

								// Get the upscale models directory from FilePathService
								std::string upscaleDir = Utils::FilePathService::GetPath("Upscale");
								if (!upscaleDir.empty() && upscaleDir[0] != '\0') {
									modelPath = (std::filesystem::path(upscaleDir) / modelName).string();
								}
								else {
									// Fallback: use executable directory
									std::string exeDir = Utils::FilePathService::GetExecutableDir();
									modelPath = (std::filesystem::path(exeDir) / "models" / "upscale_models" / modelName).string();
								}
							}

							if (esrganData.contains("upscaleFactor"))
								upscaleFactor = esrganData["upscaleFactor"].get<uint32_t>();

							// Additional ESRGAN parameters
							if (esrganData.contains("offload_params_to_cpu"))
								offload_params_to_cpu = esrganData["offload_params_to_cpu"].get<bool>();
							if (esrganData.contains("direct"))
								direct = esrganData["direct"].get<bool>();
							if (esrganData.contains("n_threads"))
								n_threads = esrganData["n_threads"].get<int>();
							if (esrganData.contains("tile_size"))
								tile_size = esrganData["tile_size"].get<int>();
						}

						// Sampler component (some upscalers might use SD context)
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];
							// Could extract parameters that affect upscaler initialization
							if (samplerData.contains("n_threads"))
								n_threads = samplerData["n_threads"].get<int>();
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

				// Check if model file exists
				if (!std::filesystem::exists(modelPath)) {
					std::cerr << "Warning: ESRGAN model file not found: " << modelPath << std::endl;

					// Try to find the model in the upscale directory
					std::string upscaleDir = Utils::FilePathService::GetPath("Upscale");
					if (!upscaleDir.empty()) {
						std::string altPath = (std::filesystem::path(upscaleDir) / std::filesystem::path(modelPath).filename()).string();
						if (std::filesystem::exists(altPath)) {
							std::cout << "Found model at alternative path: " << altPath << std::endl;
							modelPath = altPath;
						}
					}
				}

				// Load input image - force 3 channels for consistency
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData)
				{
					std::string error = std::string("Failed to load input image: ") + inputImagePath + " - " +
						(stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
					throw std::runtime_error(error);
				}

				// Force channels to 3 since we requested RGB
				inputChannels = 3;

				std::cout << "Input image loaded successfully: " << inputWidth << "x" << inputHeight
					<< " with " << inputChannels << " channels (forced RGB)" << std::endl;

				// Determine output file path
				std::string outputFilePath;
				if (!fullPath.empty())
				{
					// Use the provided fullPath if available
					outputFilePath = fullPath;
				}
				else
				{
					// Otherwise create a unique filename
					std::filesystem::path outputDir(outputPath);
					std::filesystem::path outputFile(outputFilename);

					// Check if output directory exists, if not use a fallback
					if (outputPath.empty() || outputPath[0] == '\0' || !std::filesystem::exists(outputDir)) {
						// Try to use OutputFolder from FilePathService
						std::string outputFolder = Utils::FilePathService::GetPath("OutputFolder");
						if (!outputFolder.empty() && outputFolder[0] != '\0' && std::filesystem::exists(outputFolder)) {
							outputDir = outputFolder;
						}
						else {
							// Fallback to executable directory
							outputDir = Utils::FilePathService::GetExecutableDir();
						}
					}

					outputFilePath = Utils::PngMetadata::CreateUniqueFilename(
						outputFile.string(), outputDir.string());
				}

				std::cout << "Output will be saved to: " << outputFilePath << std::endl;

				// Initialize upscaler context
				std::cout << "Initializing upscaler with model: " << modelPath << std::endl;
				std::cout << "Parameters: threads=" << n_threads
					<< ", offload_cpu=" << offload_params_to_cpu
					<< ", direct=" << direct
					<< ", tile_size=" << tile_size << std::endl;

				upscaler_context = new_upscaler_ctx(modelPath.c_str(),
					offload_params_to_cpu,
					direct,
					n_threads,
					tile_size);

				if (!upscaler_context)
				{
					throw std::runtime_error("Failed to initialize upscaler context!");
				}

				// Check actual upscale factor from context
				int actual_upscale_factor = get_upscale_factor(upscaler_context);
				if (actual_upscale_factor > 0 && actual_upscale_factor != upscaleFactor) {
					std::cout << "Note: Model supports upscale factor " << actual_upscale_factor
						<< ", overriding requested factor " << upscaleFactor << std::endl;
					upscaleFactor = actual_upscale_factor;
				}

				// Create input image struct
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					static_cast<uint32_t>(inputChannels),
					inputData
				};

				// Perform upscaling
				std::cout << "Upscaling image with factor: " << upscaleFactor << std::endl;
				upscaled_image = upscale(upscaler_context, input_image, upscaleFactor);
				if (!upscaled_image.data)
				{
					throw std::runtime_error("Upscaling failed - no output image produced");
				}

				std::cout << "Upscaling successful: " << upscaled_image.width << "x"
					<< upscaled_image.height << "x" << upscaled_image.channel << std::endl;

				// Save the upscaled image
				SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
					upscaled_image.channel, metadata, outputFilePath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				// Free the upscaled image
				if (upscaled_image.data)
				{
					free(upscaled_image.data);
					upscaled_image = { 0, 0, 0, nullptr };
				}

				// Cleanup upscaler context
				if (upscaler_context)
				{
					free_upscaler_ctx(upscaler_context);
					upscaler_context = nullptr;
				}

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

				if (upscaled_image.data)
				{
					free(upscaled_image.data);
					upscaled_image = { 0, 0, 0, nullptr };
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
} // namespace Utils