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
	void InitializeSampleParams(sd_sample_params_t &sample_params, const nlohmann::json &metadata);

	class Edit
	{
	public:
		static bool RunEdit(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			std::vector<unsigned char*> refImageData;
			sd_image_t *result_image = nullptr;

			try
			{
				// Initialize image generation parameters with defaults
				sd_img_gen_params_t gen_params;
				sd_img_gen_params_init(&gen_params);

				// Extract parameters from metadata - FIXED: Use local strings
				std::vector<std::string> refImagePaths;
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "edit_output.png";
				std::string posPrompt = "";
				std::string negPrompt = "";
				std::string idImagesPath = "";

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

						// Prompt component - FIXED: Use local strings
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
								gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();
						}

						// Latent component
						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];

							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								gen_params.width = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								gen_params.height = latentData["latentHeight"].get<int>();
						}

						// ControlNet component for edit operations
						if (comp.contains("Controlnet"))
						{
							nlohmann::json controlData = comp["Controlnet"];

							if (controlData.contains("control_strength") && !controlData["control_strength"].is_null())
								gen_params.control_strength = controlData["control_strength"].get<float>();
							if (controlData.contains("style_strength") && !controlData["style_strength"].is_null())
								gen_params.style_strength = controlData["style_strength"].get<float>();
							if (controlData.contains("normalize_input") && !controlData["normalize_input"].is_null())
								gen_params.normalize_input = controlData["normalize_input"].get<bool>();
						}

						// ID Images path for PhotoMaker/Chroma - FIXED: Use local string
						if (comp.contains("IdImages"))
						{
							nlohmann::json idImagesData = comp["IdImages"];

							if (idImagesData.contains("path") && !idImagesData["path"].is_null())
								idImagesPath = idImagesData["path"].get<std::string>();
						}
					}
				}

				// Set prompt and ID images path strings - ALWAYS use c_str(), NEVER nullptr
				gen_params.prompt = posPrompt.c_str();
				gen_params.negative_prompt = negPrompt.c_str();
				gen_params.input_id_images_path = idImagesPath.c_str();

				// Initialize sample parameters
				InitializeSampleParams(gen_params.sample_params, metadata);

				// Validate parameters
				if (refImagePaths.empty())
				{
					throw std::runtime_error("No reference images provided for edit operation!");
				}

				// Load reference images - NEW API uses array in gen_params
				std::vector<sd_image_t> ref_images;
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
						imageData
					};

					ref_images.push_back(ref_img);
				}

				// Set reference images in generation parameters (NEW API approach)
				if (!ref_images.empty())
				{
					// Use the first reference image as init_image for edit operations
					gen_params.init_image = ref_images[0];

					// Set additional reference images if available
					if (ref_images.size() > 1)
					{
						// For the new API, we need to allocate and set the ref_images array
						static std::vector<sd_image_t> static_ref_images;
						static_ref_images = std::vector<sd_image_t>(ref_images.begin() + 1, ref_images.end());
						gen_params.ref_images = static_ref_images.data();
						gen_params.ref_images_count = static_cast<int>(static_ref_images.size());
					}
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
				if (gen_params.seed < 0)
				{
					gen_params.seed = static_cast<int64_t>(generateRandomSeed());
					std::cout << "Generated random seed: " << gen_params.seed << std::endl;
				}

				// Perform edit operation using NEW structured API
				std::cout << "Calling generate_image for edit with " << ref_images.size() << " reference images..." << std::endl;
				result_image = generate_image(sd_context, &gen_params);

				if (!result_image)
				{
					throw std::runtime_error("generate_image failed - no output image produced");
				}

				if (!result_image->data)
				{
					free(result_image);
					throw std::runtime_error("generate_image produced invalid image data");
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