#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include "ContextUtils.hpp"
#include "PngMetadataUtils.hpp"
#include "FilePathService.hpp"
#include "sdcpp_utils/SchedulerUtil.hpp"
#include "sdcpp_utils/SDGuidanceUtil.hpp"
#include "sdcpp_utils/SLGUtil.hpp"
#include "sdcpp_utils/SDImageUtil.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <filesystem>

namespace Utils
{
	// Forward declarations for shared utilities
	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char* data, int width, int height, int channels,
		const nlohmann::json& metadata, const std::string& fullPath);

	class Txt2Img
	{
	public:
		// Main inference function using cached SD context
		static bool RunInference(const nlohmann::json& metadata, const std::string& fullPath, sd_ctx_t* sd_context = nullptr)
		{
			bool contextProvided = (sd_context != nullptr);
			sd_image_t* result_images = nullptr;
			sd_img_gen_params_t gen_params;
			sd_img_gen_params_init(&gen_params);

			// Resource management containers
			std::vector<std::unique_ptr<float[]>> floatArrayResources;
			std::vector<std::unique_ptr<int[]>> intArrayResources;
			std::vector<std::unique_ptr<sd_image_t>> imageResources;
			std::vector<sd_image_t> imagesToCleanup; // For backward compatibility

			try
			{
				// Parse all parameters using the utility functions
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto& comp : metadata["components"])
					{
						// Prompt component
						if (comp.contains("Prompt"))
						{
							nlohmann::json promptData = comp["Prompt"];
							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								gen_params.prompt = promptData["posPrompt"].get<std::string>().c_str();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								gen_params.negative_prompt = promptData["negPrompt"].get<std::string>().c_str();
						}

						// ClipSkip component
						if (comp.contains("ClipSkip"))
						{
							nlohmann::json clipSkipData = comp["ClipSkip"];
							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						// Sampler component - use SchedulerUtil
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];

							// Basic sampler parameters
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();
							else
								gen_params.strength = 1.0f;

							// Parse sample parameters
							ParseSampleParams(comp, gen_params.sample_params, floatArrayResources);
						}

						// Guidance component - use SDGuidanceUtil
						if (comp.contains("Guidance"))
						{
							ParseGuidanceParams(comp, gen_params.sample_params.guidance);
						}

						// SLG component - use SLGUtil
						if (comp.contains("SLG"))
						{
							ParseSLGParams(comp, gen_params.sample_params.guidance.slg, intArrayResources);
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

						// ControlNet component
						if (comp.contains("Controlnet"))
						{
							nlohmann::json controlNetData = comp["Controlnet"];
							if (controlNetData.contains("cnStrength") && !controlNetData["cnStrength"].is_null())
							{
								gen_params.control_strength = controlNetData["cnStrength"].get<float>();
							}
						}

						// VAE component - read all tiling parameters
						if (comp.contains("Vae"))
						{
							nlohmann::json vaeData = comp["Vae"];
							if (vaeData.contains("isTiled") && !vaeData["isTiled"].is_null())
								gen_params.vae_tiling_params.enabled = vaeData["isTiled"].get<bool>();
							if (vaeData.contains("tile_size_x") && !vaeData["tile_size_x"].is_null())
								gen_params.vae_tiling_params.tile_size_x = vaeData["tile_size_x"].get<int>();
							if (vaeData.contains("tile_size_y") && !vaeData["tile_size_y"].is_null())
								gen_params.vae_tiling_params.tile_size_y = vaeData["tile_size_y"].get<int>();
							if (vaeData.contains("target_overlap") && !vaeData["target_overlap"].is_null())
								gen_params.vae_tiling_params.target_overlap = vaeData["target_overlap"].get<float>();
							if (vaeData.contains("rel_size_x") && !vaeData["rel_size_x"].is_null())
								gen_params.vae_tiling_params.rel_size_x = vaeData["rel_size_x"].get<float>();
							if (vaeData.contains("rel_size_y") && !vaeData["rel_size_y"].is_null())
								gen_params.vae_tiling_params.rel_size_y = vaeData["rel_size_y"].get<float>();
						}

						// PhotoMaker component
						if (comp.contains("PhotoMaker"))
						{
							nlohmann::json pmData = comp["PhotoMaker"];
							if (pmData.contains("modelPath") && !pmData["modelPath"].is_null())
							{
								std::string pm_path = pmData["modelPath"].get<std::string>();
								gen_params.pm_params.id_embed_path = pm_path.c_str();
							}
							if (pmData.contains("style_strength") && !pmData["style_strength"].is_null())
							{
								gen_params.pm_params.style_strength = pmData["style_strength"].get<float>();
							}
						}

						// ControlNet Image component - use SDImageUtil
						if (comp.contains("ControlNetImage"))
						{
							sd_image_t control_image = { 0, 0, 0, nullptr };
							if (ParseImageComponent(comp, control_image, imageResources))
							{
								gen_params.control_image = control_image;

								// Get control strength from image component if not set from ControlNet component
								nlohmann::json controlImageData = comp["ControlNetImage"];
								if (controlImageData.contains("strength") && !controlImageData["strength"].is_null() &&
									gen_params.control_strength == 0.0f)
								{
									gen_params.control_strength = controlImageData["strength"].get<float>();
								}
							}
						}

						// PhotoMaker ID Images - can have multiple
						if (comp.contains("PhotoMakerImage"))
						{
							sd_image_t id_image = { 0, 0, 0, nullptr };
							if (ParseImageComponent(comp, id_image, imageResources))
							{
								// We need to collect multiple ID images
								static std::vector<sd_image_t> idImagesStorage;
								idImagesStorage.push_back(id_image);
								gen_params.pm_params.id_images = idImagesStorage.data();
								gen_params.pm_params.id_images_count = idImagesStorage.size();
							}
						}

						// Reference Image component
						if (comp.contains("ReferenceImage"))
						{
							sd_image_t ref_image = { 0, 0, 0, nullptr };
							if (ParseImageComponent(comp, ref_image, imageResources))
							{
								static std::vector<sd_image_t> refImagesStorage;
								refImagesStorage.push_back(ref_image);
								gen_params.ref_images = refImagesStorage.data();
								gen_params.ref_images_count = refImagesStorage.size();
							}

							nlohmann::json refImageData = comp["ReferenceImage"];
							if (refImageData.contains("autoResize") && !refImageData["autoResize"].is_null())
							{
								gen_params.auto_resize_ref_image = refImageData["autoResize"].get<bool>();
							}
							if (refImageData.contains("increaseIndex") && !refImageData["increaseIndex"].is_null())
							{
								gen_params.increase_ref_index = refImageData["increaseIndex"].get<bool>();
							}
						}

						// Mask Image component
						if (comp.contains("MaskImage"))
						{
							sd_image_t mask_image = { 0, 0, 0, nullptr };
							if (ParseImageComponent(comp, mask_image, imageResources))
							{
								gen_params.mask_image = mask_image;
							}
						}

						// Init Image component
						if (comp.contains("InitImage"))
						{
							sd_image_t init_image = { 0, 0, 0, nullptr };
							if (ParseImageComponent(comp, init_image, imageResources))
							{
								gen_params.init_image = init_image;
							}
						}
					}
				}

				// Get SD context if not provided
				if (!contextProvided) {
					sd_context = SDContextManager::GetOrCreateContext(metadata);
				}

				if (!sd_context)
				{
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Ensure valid seed
				if (gen_params.seed < 0)
				{
					gen_params.seed = static_cast<int64_t>(generateRandomSeed());
				}

				// Perform txt2img
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

				// Save the result image
				SaveImage(result_images->data, result_images->width, result_images->height,
					result_images->channel, metadata, fullPath);

				// Cleanup
				if (result_images)
				{
					free(result_images);
				}

				// Release context back to cache if we acquired it
				if (!contextProvided) {
					SDContextManager::ReleaseContext(sd_context);
				}

				return true;
			}
			catch (const std::exception& e)
			{
				std::cerr << "Exception during txt2img: " << e.what() << std::endl;

				// Cleanup result images
				if (result_images)
				{
					free(result_images);
				}

				// Release context back to cache if we acquired it
				if (!contextProvided && sd_context) {
					SDContextManager::ReleaseContext(sd_context);
				}

				return false;
			}
		}
	};
} // namespace Utils