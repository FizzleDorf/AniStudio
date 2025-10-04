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
				bool offload_params_to_cpu = false;
				bool direct = false;
				int n_threads = 4;

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
								modelPath = (std::filesystem::path(Utils::FilePaths::upscaleDir) / modelName).string();
							}

							if (esrganData.contains("upscaleFactor"))
								upscaleFactor = esrganData["upscaleFactor"].get<uint32_t>();
						}

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("n_threads"))
								n_threads = samplerData["n_threads"].get<int>();
							if (samplerData.contains("offload_params_to_cpu"))
								offload_params_to_cpu = samplerData["offload_params_to_cpu"].get<bool>();
							if (samplerData.contains("direct"))
								direct = samplerData["direct"].get<bool>();
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

				// Initialize upscaler context with NEW API signature
				upscaler_context = new_upscaler_ctx(modelPath.c_str(),
					offload_params_to_cpu,
					direct,
					n_threads);
				if (!upscaler_context)
				{
					throw std::runtime_error("Failed to initialize upscaler context!");
				}

				// Create input image struct
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					static_cast<uint32_t>(inputChannels),
					inputData
				};

				// Perform upscaling using the NEW API
				sd_image_t upscaled_image = upscale(upscaler_context, input_image, upscaleFactor);
				if (!upscaled_image.data)
				{
					throw std::runtime_error("Upscaling failed - no output image produced");
				}

				std::cout << "Upscaling successful, saving output to: " << uniqueFilePath << std::endl;

				// Save the upscaled image
				SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
					upscaled_image.channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);  // stb_image allocation
					inputData = nullptr;
				}

				// Free the upscaled image if needed
				if (upscaled_image.data)
				{
					free(upscaled_image.data);  // stable-diffusion.cpp allocation
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
					stbi_image_free(inputData);  // stb_image allocation
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
} // namespace Utils