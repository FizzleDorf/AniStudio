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

	class Edit
	{
	public:
		static bool RunEdit(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			std::vector<unsigned char*> refImageData;
			std::vector<sd_image_t> ref_images;
			sd_image_t *result_image = nullptr;

			try
			{
				// Extract parameters from metadata
				std::vector<std::string> refImagePaths;
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "edit_output.png";
				std::string posPrompt = "", negPrompt = "";
				float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
				int width = 512, height = 512;
				int steps = 20, seed = -1, batchSize = 1;
				float strength = 0.8f;
				sample_method_t sample_method = EULER;
				int *skipLayers = nullptr;
				size_t skipLayersCount = 0;
				float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;
				float control_strength = 0.0f, style_strength = 0.0f;
				bool normalize_input = false;

				// Debug logging for metadata
				std::cout << "Edit metadata:" << std::endl;
				std::cout << metadata.dump(2) << std::endl;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						// Reference images - could be multiple for editing
						if (comp.contains("ReferenceImages"))
						{
							nlohmann::json refImagesData = comp["ReferenceImages"];

							if (refImagesData.is_array())
							{
								for (const auto &refImg : refImagesData)
								{
									if (refImg.contains("filePath") && !refImg["filePath"].is_null() && !refImg["filePath"].get<std::string>().empty())
									{
										refImagePaths.push_back(refImg["filePath"].get<std::string>());
										std::cout << "Found reference image path: " << refImg["filePath"].get<std::string>() << std::endl;
									}
								}
							}
							else if (refImagesData.contains("filePath") && !refImagesData["filePath"].is_null())
							{
								// Single reference image
								refImagePaths.push_back(refImagesData["filePath"].get<std::string>());
								std::cout << "Found single reference image path: " << refImagesData["filePath"].get<std::string>() << std::endl;
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
								strength = samplerData["denoise"].get<float>();
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
								width = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								height = latentData["latentHeight"].get<int>();
							if (latentData.contains("batchSize") && !latentData["batchSize"].is_null())
								batchSize = latentData["batchSize"].get<int>();
						}

						// ControlNet component for edit operations
						if (comp.contains("Controlnet"))
						{
							nlohmann::json controlData = comp["Controlnet"];

							if (controlData.contains("control_strength") && !controlData["control_strength"].is_null())
								control_strength = controlData["control_strength"].get<float>();
							if (controlData.contains("style_strength") && !controlData["style_strength"].is_null())
								style_strength = controlData["style_strength"].get<float>();
							if (controlData.contains("normalize_input") && !controlData["normalize_input"].is_null())
								normalize_input = controlData["normalize_input"].get<bool>();
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
				if (refImagePaths.empty())
				{
					throw std::runtime_error("No reference images provided for edit operation!");
				}

				// Load reference images
				for (const std::string& imagePath : refImagePaths)
				{
					if (!std::filesystem::exists(imagePath))
					{
						throw std::runtime_error("Reference image file does not exist: " + imagePath);
					}

					int imgWidth, imgHeight, imgChannels;
					std::cout << "Loading reference image from: " << imagePath << std::endl;

					// Force 3 channels (RGB) for consistency
					unsigned char* imageData = stbi_load(imagePath.c_str(), &imgWidth, &imgHeight, &imgChannels, 3);
					if (!imageData)
					{
						std::string error = std::string("Failed to load reference image: ") + imagePath + " - " +
							(stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
						throw std::runtime_error(error);
					}

					// Force channels to 3 since we requested RGB
					imgChannels = 3;

					std::cout << "Reference image loaded successfully: " << imgWidth << "x" << imgHeight
						<< " with " << imgChannels << " channels (forced RGB)" << std::endl;

					// Validate image dimensions
					if (imgWidth <= 0 || imgHeight <= 0)
					{
						stbi_image_free(imageData);
						throw std::runtime_error("Invalid reference image dimensions: " + std::to_string(imgWidth) + "x" + std::to_string(imgHeight));
					}

					// Store the image data and create sd_image_t struct
					refImageData.push_back(imageData);

					sd_image_t ref_img = {
						static_cast<uint32_t>(imgWidth),
						static_cast<uint32_t>(imgHeight),
						3, // Force 3 channels (RGB)
						imageData };

					ref_images.push_back(ref_img);
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

				// Ensure valid seed
				if (seed < 0)
				{
					seed = static_cast<int>(generateRandomSeed());
					std::cout << "Generated random seed: " << seed << std::endl;
				}

				// Perform edit operation
				std::cout << "Calling edit with " << ref_images.size() << " reference images..." << std::endl;
				result_image = edit(
					sd_context,
					ref_images.data(),
					static_cast<int>(ref_images.size()),
					posPrompt.c_str(),
					negPrompt.c_str(),
					static_cast<int>(clipSkip),
					cfg,
					guidance,
					eta,
					width,
					height,
					sample_method,
					steps,
					strength,
					static_cast<int64_t>(seed),
					batchSize,
					nullptr, // control_cond
					control_strength,
					style_strength,
					normalize_input,
					skipLayers,
					skipLayersCount,
					slgScale,
					skipLayerStart,
					skipLayerEnd);

				if (!result_image)
				{
					throw std::runtime_error("edit failed - no output image produced");
				}

				if (!result_image->data)
				{
					free(result_image);
					throw std::runtime_error("edit produced invalid image data");
				}

				std::cout << "edit successful: " << result_image->width << "x" << result_image->height
					<< "x" << result_image->channel << ", saving to: " << fullPath << std::endl;

				// Save the result image
				SaveImage(result_image->data, result_image->width, result_image->height,
					result_image->channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup resources
				for (unsigned char* imageData : refImageData)
				{
					if (imageData)
					{
						stbi_image_free(imageData);
					}
				}
				refImageData.clear();
				ref_images.clear();

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
				std::cerr << "Exception during edit: " << e.what() << std::endl;

				// Clean up resources
				for (unsigned char* imageData : refImageData)
				{
					if (imageData)
					{
						stbi_image_free(imageData);
					}
				}
				refImageData.clear();
				ref_images.clear();

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