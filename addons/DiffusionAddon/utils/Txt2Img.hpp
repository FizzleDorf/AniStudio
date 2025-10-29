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

	class Txt2Img
	{
	public:
		// Main inference function using NEW structured API
		static bool RunInference(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			sd_image_t *result_images = nullptr;
			sd_img_gen_params_t gen_params;
			sd_img_gen_params_init(&gen_params);
			
			std::cout << "=== METADATA STRUCTURE DEBUG ===" << std::endl;
			std::cout << "Full metadata: " << metadata.dump(2) << std::endl;
			std::cout << "=== END METADATA DEBUG ===" << std::endl;
			try
			{
				// Extract parameters from metadata - use local strings for proper lifetime
				std::string outputPath = Utils::FilePaths::defaultProjectPath;
				std::string outputFilename = "txt2img_output.png";
				std::string posPrompt = "";
				std::string negPrompt = "";

				// Debug logging for metadata
				std::cout << "Txt2Img metadata:" << std::endl;
				std::cout << metadata.dump(2) << std::endl;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
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
								gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						// Sampler component - extract core sampling parameters
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();

							// Extract sample_params fields
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								gen_params.sample_params.sample_steps = samplerData["steps"].get<int>();
							if (samplerData.contains("eta") && !samplerData["eta"].is_null())
								gen_params.sample_params.eta = samplerData["eta"].get<float>();
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();

							// Extract method selections
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								gen_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
							if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
								gen_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
						}

						// Guidance component - map to sample_params.guidance
						if (comp.contains("Guidance"))
						{
							nlohmann::json guidanceData = comp["Guidance"];

							if (guidanceData.contains("txt_cfg") && !guidanceData["txt_cfg"].is_null())
								gen_params.sample_params.guidance.txt_cfg = guidanceData["txt_cfg"].get<float>();
							if (guidanceData.contains("img_cfg") && !guidanceData["img_cfg"].is_null())
								gen_params.sample_params.guidance.img_cfg = guidanceData["img_cfg"].get<float>();
							if (guidanceData.contains("distilled_guidance") && !guidanceData["distilled_guidance"].is_null())
								gen_params.sample_params.guidance.distilled_guidance = guidanceData["distilled_guidance"].get<float>();
						}

						// SLG component - map to sample_params.guidance.slg
						if (comp.contains("SLG"))
						{
							nlohmann::json slgData = comp["SLG"];

							if (slgData.contains("layer_start") && !slgData["layer_start"].is_null())
								gen_params.sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
							if (slgData.contains("layer_end") && !slgData["layer_end"].is_null())
								gen_params.sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
							if (slgData.contains("scale") && !slgData["scale"].is_null())
								gen_params.sample_params.guidance.slg.scale = slgData["scale"].get<float>();

							// Handle layers array for SLG
							if (slgData.contains("layers") && slgData["layers"].is_array() &&
								slgData.contains("layer_count") && !slgData["layer_count"].is_null())
							{
								size_t layer_count = slgData["layer_count"].get<size_t>();
								if (layer_count > 0 && slgData["layers"].size() >= layer_count)
								{
									// Allocate and copy layers array
									gen_params.sample_params.guidance.slg.layers = new int[layer_count];
									gen_params.sample_params.guidance.slg.layer_count = layer_count;

									for (size_t i = 0; i < layer_count; i++)
									{
										gen_params.sample_params.guidance.slg.layers[i] = slgData["layers"][i].get<int>();
									}
								}
							}
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
					}
				}

				// Set prompt strings - ALWAYS use c_str(), NEVER nullptr
				gen_params.prompt = posPrompt.c_str();
				gen_params.negative_prompt = negPrompt.c_str();

				// Initialize empty image structs for txt2img (no input images needed)
				gen_params.init_image = { 0, 0, 0, nullptr };
				gen_params.mask_image = { 0, 0, 0, nullptr };
				gen_params.control_image = { 0, 0, 0, nullptr };
				gen_params.ref_images = nullptr;
				gen_params.ref_images_count = 0;
				gen_params.increase_ref_index = false;

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

				// Perform txt2img using NEW structured API
				std::cout << "Calling generate_image for txt2img generation..." << std::endl;
				result_images = generate_image(sd_context, &gen_params);

				if (!result_images)
				{
					throw std::runtime_error("generate_image failed - no output image produced");
				}

				if (!result_images->data)
				{
					free(result_images);
					throw std::runtime_error("generate_image produced invalid image data");
				}

				std::cout << "generate_image successful: " << result_images->width << "x" << result_images->height
					<< "x" << result_images->channel << ", saving to: " << fullPath << std::endl;

				// Save the result image
				SaveImage(result_images->data, result_images->width, result_images->height,
					result_images->channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));

				// Cleanup SLG layers array if it was allocated
				if (gen_params.sample_params.guidance.slg.layers != nullptr)
				{
					delete[] gen_params.sample_params.guidance.slg.layers;
					gen_params.sample_params.guidance.slg.layers = nullptr;
				}

				// Cleanup resources
				if (result_images)
				{
					free(result_images);
					result_images = nullptr;
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
				std::cerr << "Exception during txt2img: " << e.what() << std::endl;

				// Clean up SLG layers array if it was allocated
				if (gen_params.sample_params.guidance.slg.layers != nullptr)
				{
					delete[] gen_params.sample_params.guidance.slg.layers;
					gen_params.sample_params.guidance.slg.layers = nullptr;
				}

				// Clean up resources
				if (result_images)
				{
					free(result_images);
					result_images = nullptr;
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