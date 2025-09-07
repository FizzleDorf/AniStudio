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
	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);
	sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata);

	class Img2Img
	{
	public:
		static bool RunImg2Img(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			unsigned char *inputData = nullptr;
			unsigned char *maskData = nullptr;
			unsigned char *emptyMaskData = nullptr;
			sd_image_t *result_image = nullptr;

			try
			{
				// Extract parameters from metadata
				std::string inputImagePath = "";
				std::string maskImagePath = "";
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "img2img_output.png";
				std::string posPrompt = "", negPrompt = "";
				float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
				int latentWidth = 512, latentHeight = 512;
				int steps = 20, seed = -1, batchSize = 1;
				float denoiseStrength = 0.75f;
				sample_method_t sample_method = EULER;
				int *skipLayers = nullptr;
				size_t skipLayersCount = 0;
				float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;

				// Debug logging for metadata
				std::cout << "Img2Img metadata:" << std::endl;
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

						// Mask image path (additional for img2img)
						if (comp.contains("MaskImage"))
						{
							nlohmann::json maskImageData = comp["MaskImage"];

							if (maskImageData.contains("filePath") && !maskImageData["filePath"].is_null() && !maskImageData["filePath"].get<std::string>().empty())
							{
								maskImagePath = maskImageData["filePath"].get<std::string>();
								std::cout << "Found mask path: " << maskImagePath << std::endl;
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

						// Prompt component
						if (comp.contains("Prompt"))
						{
							nlohmann::json promptData = comp["Prompt"];

							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								posPrompt = promptData["posPrompt"].get<std::string>();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								negPrompt = promptData["negPrompt"].get<std::string>();
						}

						// ClipSkip component
						if (comp.contains("ClipSkip"))
						{
							nlohmann::json clipSkipData = comp["ClipSkip"];

							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								clipSkip = clipSkipData["clipSkip"].get<float>();
						}

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("cfg") && !samplerData["cfg"].is_null())
								cfg = samplerData["cfg"].get<float>();
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								steps = samplerData["steps"].get<int>();
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								seed = samplerData["seed"].get<int>();
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								denoiseStrength = samplerData["denoise"].get<float>();
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
						}

						// Guidance component
						if (comp.contains("Guidance"))
						{
							nlohmann::json guidanceData = comp["Guidance"];

							if (guidanceData.contains("guidance") && !guidanceData["guidance"].is_null())
								guidance = guidanceData["guidance"].get<float>();
							if (guidanceData.contains("eta") && !guidanceData["eta"].is_null())
								eta = guidanceData["eta"].get<float>();
						}

						// Latent component
						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];

							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								latentWidth = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								latentHeight = latentData["latentHeight"].get<int>();
							if (latentData.contains("batchSize") && !latentData["batchSize"].is_null())
								batchSize = latentData["batchSize"].get<int>();
						}

						// Layer Skip component
						if (comp.contains("LayerSkip"))
						{
							nlohmann::json layerSkipData = comp["LayerSkip"];

							if (layerSkipData.contains("slg_scale") && !layerSkipData["slg_scale"].is_null())
								slgScale = layerSkipData["slg_scale"].get<float>();
							if (layerSkipData.contains("skip_layer_start") && !layerSkipData["skip_layer_start"].is_null())
								skipLayerStart = layerSkipData["skip_layer_start"].get<float>();
							if (layerSkipData.contains("skip_layer_end") && !layerSkipData["skip_layer_end"].is_null())
								skipLayerEnd = layerSkipData["skip_layer_end"].get<float>();
						}
					}
				}

				// Validate parameters
				if (inputImagePath.empty())
				{
					throw std::runtime_error("Input image path is empty!");
				}

				// Check if input image file exists
				if (!std::filesystem::exists(inputImagePath))
				{
					throw std::runtime_error("Input image file does not exist: " + inputImagePath);
				}

				// Load input image
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;

				// Force 3 channels (RGB) for consistency with stable-diffusion
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

				// Validate image dimensions
				if (inputWidth <= 0 || inputHeight <= 0)
				{
					throw std::runtime_error("Invalid input image dimensions: " + std::to_string(inputWidth) + "x" + std::to_string(inputHeight));
				}

				// Create output path
				std::filesystem::path outputDir(outputPath);
				std::filesystem::path outputFile(outputFilename);
				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputFile.string(), outputDir.string());

				// Initialize SD context
				std::cout << "Initializing Stable Diffusion context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context)
				{
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Create input image struct - FORCE RGB FORMAT
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3, // FORCE 3 channels (RGB)
					inputData };

				// Initialize mask image struct - IMPROVED MASK HANDLING
				sd_image_t mask_image = { 0 };

				if (maskImagePath.empty() || !std::filesystem::exists(maskImagePath))
				{
					std::cout << "No valid mask provided, creating blank white mask" << std::endl;

					// Create a blank WHITE mask (255 = keep original, 0 = replace with generated)
					size_t maskSize = inputWidth * inputHeight;
					emptyMaskData = new unsigned char[maskSize];
					std::memset(emptyMaskData, 255, maskSize); // WHITE mask = change whole image

					mask_image.width = static_cast<uint32_t>(inputWidth);
					mask_image.height = static_cast<uint32_t>(inputHeight);
					mask_image.channel = 1; // Masks are single channel
					mask_image.data = emptyMaskData;

					std::cout << "Created white mask: " << inputWidth << "x" << inputHeight << std::endl;
				}
				else
				{
					// Load mask from file - FORCE GRAYSCALE
					int maskWidth, maskHeight, maskChannels;
					std::cout << "Loading mask image from: " << maskImagePath << std::endl;

					// FORCE 1 channel (grayscale) for mask
					maskData = stbi_load(maskImagePath.c_str(), &maskWidth, &maskHeight, &maskChannels, 1);

					if (!maskData)
					{
						std::cerr << "Failed to load mask image: " << maskImagePath << ", using white mask instead" << std::endl;

						// Create white mask as fallback
						size_t maskSize = inputWidth * inputHeight;
						emptyMaskData = new unsigned char[maskSize];
						std::memset(emptyMaskData, 255, maskSize); // WHITE mask

						mask_image.width = static_cast<uint32_t>(inputWidth);
						mask_image.height = static_cast<uint32_t>(inputHeight);
						mask_image.channel = 1;
						mask_image.data = emptyMaskData;
					}
					else
					{
						// Validate mask dimensions match input image
						if (maskWidth != inputWidth || maskHeight != inputHeight)
						{
							std::cout << "Warning: Mask dimensions (" << maskWidth << "x" << maskHeight
								<< ") don't match input image (" << inputWidth << "x" << inputHeight
								<< "), using white mask instead" << std::endl;

							// Free the loaded mask and create a white one
							stbi_image_free(maskData);
							maskData = nullptr;

							size_t maskSize = inputWidth * inputHeight;
							emptyMaskData = new unsigned char[maskSize];
							std::memset(emptyMaskData, 255, maskSize);

							mask_image.width = static_cast<uint32_t>(inputWidth);
							mask_image.height = static_cast<uint32_t>(inputHeight);
							mask_image.channel = 1;
							mask_image.data = emptyMaskData;
						}
						else
						{
							mask_image.width = static_cast<uint32_t>(maskWidth);
							mask_image.height = static_cast<uint32_t>(maskHeight);
							mask_image.channel = 1; // Force single channel
							mask_image.data = maskData;

							std::cout << "Mask image loaded successfully: " << maskWidth << "x" << maskHeight
								<< " with 1 channel (grayscale)" << std::endl;
						}
					}
				}

				// Ensure valid seed
				if (seed < 0)
				{
					seed = static_cast<int>(generateRandomSeed());
					std::cout << "Generated random seed: " << seed << std::endl;
				}

				// Perform img2img
				std::cout << "Calling img2img..." << std::endl;
				result_image = img2img(
					sd_context,
					input_image,
					mask_image,
					posPrompt.c_str(),
					negPrompt.c_str(),
					static_cast<int>(clipSkip),
					cfg,
					guidance,
					eta,
					latentWidth,
					latentHeight,
					sample_method,
					steps,
					denoiseStrength,
					static_cast<int64_t>(seed),
					batchSize,
					nullptr, // control_cond
					0.0f,	 // control_strength
					0.0f,	 // style_ratio
					false,	 // normalize_input
					"",		 // input_id_images_path
					skipLayers,
					skipLayersCount,
					slgScale,
					skipLayerStart,
					skipLayerEnd);

				if (!result_image)
				{
					throw std::runtime_error("img2img failed - no output image produced");
				}

				if (!result_image->data)
				{
					free(result_image);
					throw std::runtime_error("img2img produced invalid image data");
				}

				std::cout << "img2img successful: " << result_image->width << "x" << result_image->height
					<< "x" << result_image->channel << ", saving to: " << fullPath << std::endl;

				// Save the result image
				SaveImage(result_image->data, result_image->width, result_image->height,
					result_image->channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (maskData)
				{
					stbi_image_free(maskData);
					maskData = nullptr;
				}

				if (emptyMaskData)
				{
					delete[] emptyMaskData;
					emptyMaskData = nullptr;
				}

				if (result_image)
				{
					free(result_image);
					result_image = nullptr;
				}

				if (sd_context)
				{
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during img2img: " << e.what() << std::endl;

				// Clean up resources
				if (inputData)
				{
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (maskData)
				{
					stbi_image_free(maskData);
					maskData = nullptr;
				}

				if (emptyMaskData)
				{
					delete[] emptyMaskData;
					emptyMaskData = nullptr;
				}

				if (result_image)
				{
					free(result_image);
					result_image = nullptr;
				}

				if (sd_context)
				{
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return false;
			}
		}
	};
} // namespace Utils