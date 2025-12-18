#pragma once
#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "FilePaths.hpp"
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

	// Helper function to load image from file to sd_image_t
	inline sd_image_t LoadImageToSDImage(const std::string& filePath) {
		sd_image_t result = { 0, 0, 0, nullptr };

		if (filePath.empty() || !std::filesystem::exists(filePath)) {
			return result;
		}

		int width, height, channels;
		unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
		if (data) {
			result.width = width;
			result.height = height;
			result.channel = channels;
			result.data = data; // Note: This needs to be freed with stbi_image_free
		}

		return result;
	}

	// Helper to free sd_image_t data
	inline void FreeSDImage(sd_image_t& img) {
		if (img.data) {
			stbi_image_free(img.data);
			img.data = nullptr;
		}
	}

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

			// Arrays to track images that need cleanup
			std::vector<sd_image_t> imagesToCleanup;
			std::vector<sd_image_t> idImagesStorage; // Store ID images for PhotoMaker
			std::vector<sd_image_t> refImagesStorage; // Store reference images

			// Track allocated SLG layers
			int* slg_layers = nullptr;

			try
			{
				// Get FilePaths instance
				FilePaths& filePaths = FilePaths::GetInstance();

				// Extract parameters from metadata
				std::string outputPath = filePaths.GetPath("DefaultProject");
				std::string outputFilename = "txt2img_output.png";
				std::string posPrompt = "";
				std::string negPrompt = "";

				// VAE tiling parameters
				sd_tiling_params_t vae_tiling_params = { false, 64, 64, 0.0f, 64.0f, 64.0f };

				// PhotoMaker parameters
				sd_pm_params_t pm_params = { nullptr, 0, nullptr, 0.0f };

				// ControlNet image
				sd_image_t control_image = { 0, 0, 0, nullptr };
				float control_strength = 0.0f;
				float apply_start = 0.0f;
				float apply_end = 1.0f;

				// Reference images
				std::vector<sd_image_t> refImages;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						// Output settings
						if (comp.contains("OutputImage"))
						{
							nlohmann::json outputImageData = comp["OutputImage"];
							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null())
								outputPath = outputImageData["filePath"].get<std::string>();
							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null())
								outputFilename = outputImageData["fileName"].get<std::string>();
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

						// Sampler component
						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								gen_params.sample_params.sample_steps = samplerData["steps"].get<int>();
							if (samplerData.contains("eta") && !samplerData["eta"].is_null())
								gen_params.sample_params.eta = samplerData["eta"].get<float>();
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();
							else
								gen_params.strength = 1.0f; // Full denoising for txt2img

							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								gen_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
							if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
								gen_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
							if (samplerData.contains("shifted_timestep") && !samplerData["shifted_timestep"].is_null())
								gen_params.sample_params.shifted_timestep = samplerData["shifted_timestep"].get<int>();
							if (samplerData.contains("current_prediction_type") && !samplerData["current_prediction_type"].is_null())
							{
								// Note: prediction type should be set in context params, not generation params
								// This would need to be handled during context initialization
							}
						}

						// Guidance component
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

						// SLG component
						if (comp.contains("SLG"))
						{
							nlohmann::json slgData = comp["SLG"];
							if (slgData.contains("layer_start") && !slgData["layer_start"].is_null())
								gen_params.sample_params.guidance.slg.layer_start = slgData["layer_start"].get<float>();
							if (slgData.contains("layer_end") && !slgData["layer_end"].is_null())
								gen_params.sample_params.guidance.slg.layer_end = slgData["layer_end"].get<float>();
							if (slgData.contains("scale") && !slgData["scale"].is_null())
								gen_params.sample_params.guidance.slg.scale = slgData["scale"].get<float>();

							if (slgData.contains("layers") && slgData["layers"].is_array() &&
								slgData.contains("layer_count") && !slgData["layer_count"].is_null())
							{
								size_t layer_count = slgData["layer_count"].get<size_t>();
								if (layer_count > 0 && slgData["layers"].size() >= layer_count)
								{
									slg_layers = new int[layer_count];
									gen_params.sample_params.guidance.slg.layers = slg_layers;
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

						// ControlNet component
						if (comp.contains("Controlnet"))
						{
							nlohmann::json controlNetData = comp["Controlnet"];
							if (controlNetData.contains("cnStrength") && !controlNetData["cnStrength"].is_null())
							{
								control_strength = controlNetData["cnStrength"].get<float>();
							}
							if (controlNetData.contains("applyStart") && !controlNetData["applyStart"].is_null())
							{
								apply_start = controlNetData["applyStart"].get<float>();
							}
							if (controlNetData.contains("applyEnd") && !controlNetData["applyEnd"].is_null())
							{
								apply_end = controlNetData["applyEnd"].get<float>();
							}
						}

						// VAE component - read all tiling parameters
						if (comp.contains("Vae"))
						{
							nlohmann::json vaeData = comp["Vae"];
							if (vaeData.contains("isTiled") && !vaeData["isTiled"].is_null())
								vae_tiling_params.enabled = vaeData["isTiled"].get<bool>();
							if (vaeData.contains("tile_size_x") && !vaeData["tile_size_x"].is_null())
								vae_tiling_params.tile_size_x = vaeData["tile_size_x"].get<int>();
							if (vaeData.contains("tile_size_y") && !vaeData["tile_size_y"].is_null())
								vae_tiling_params.tile_size_y = vaeData["tile_size_y"].get<int>();
							if (vaeData.contains("target_overlap") && !vaeData["target_overlap"].is_null())
								vae_tiling_params.target_overlap = vaeData["target_overlap"].get<float>();
							if (vaeData.contains("rel_size_x") && !vaeData["rel_size_x"].is_null())
								vae_tiling_params.rel_size_x = vaeData["rel_size_x"].get<float>();
							if (vaeData.contains("rel_size_y") && !vaeData["rel_size_y"].is_null())
								vae_tiling_params.rel_size_y = vaeData["rel_size_y"].get<float>();
						}

						// PhotoMaker component
						if (comp.contains("PhotoMaker"))
						{
							nlohmann::json pmData = comp["PhotoMaker"];
							if (pmData.contains("modelPath") && !pmData["modelPath"].is_null())
							{
								std::string pm_path = pmData["modelPath"].get<std::string>();
								pm_params.id_embed_path = pm_path.c_str();
							}
							if (pmData.contains("style_strength") && !pmData["style_strength"].is_null())
							{
								pm_params.style_strength = pmData["style_strength"].get<float>();
							}
						}

						// ControlNet Image component
						if (comp.contains("ControlNetImage"))
						{
							nlohmann::json controlImageData = comp["ControlNetImage"];

							// Load control image
							if (controlImageData.contains("filePath") && !controlImageData["filePath"].is_null() &&
								!controlImageData["filePath"].get<std::string>().empty())
							{
								std::string controlImagePath = controlImageData["filePath"].get<std::string>();
								control_image = LoadImageToSDImage(controlImagePath);
								if (control_image.data) {
									imagesToCleanup.push_back(control_image);
								}
							}

							// Get control strength from image component if not set from ControlNet component
							if (controlImageData.contains("strength") && !controlImageData["strength"].is_null() && control_strength == 0.0f)
							{
								control_strength = controlImageData["strength"].get<float>();
							}
						}

						// PhotoMaker ID Images - can have multiple
						if (comp.contains("PhotoMakerImage"))
						{
							nlohmann::json pmImageData = comp["PhotoMakerImage"];

							if (pmImageData.contains("filePath") && !pmImageData["filePath"].is_null() &&
								!pmImageData["filePath"].get<std::string>().empty())
							{
								std::string idImagePath = pmImageData["filePath"].get<std::string>();
								sd_image_t id_image = LoadImageToSDImage(idImagePath);
								if (id_image.data) {
									idImagesStorage.push_back(id_image);
									imagesToCleanup.push_back(id_image);
								}
							}
						}

						// Reference Image component (for img2img or style transfer)
						if (comp.contains("ReferenceImage"))
						{
							nlohmann::json refImageData = comp["ReferenceImage"];

							if (refImageData.contains("filePath") && !refImageData["filePath"].is_null() &&
								!refImageData["filePath"].get<std::string>().empty())
							{
								std::string refImagePath = refImageData["filePath"].get<std::string>();
								sd_image_t ref_image = LoadImageToSDImage(refImagePath);
								if (ref_image.data) {
									refImagesStorage.push_back(ref_image);
									imagesToCleanup.push_back(ref_image);
								}
							}

							// Get reference image settings
							if (refImageData.contains("autoResize") && !refImageData["autoResize"].is_null())
							{
								gen_params.auto_resize_ref_image = refImageData["autoResize"].get<bool>();
							}
							if (refImageData.contains("increaseIndex") && !refImageData["increaseIndex"].is_null())
							{
								gen_params.increase_ref_index = refImageData["increaseIndex"].get<bool>();
							}
						}

						// Mask Image component (for inpainting)
						if (comp.contains("MaskImage"))
						{
							nlohmann::json maskImageData = comp["MaskImage"];

							if (maskImageData.contains("filePath") && !maskImageData["filePath"].is_null() &&
								!maskImageData["filePath"].get<std::string>().empty())
							{
								std::string maskImagePath = maskImageData["filePath"].get<std::string>();
								sd_image_t mask_image = LoadImageToSDImage(maskImagePath);
								if (mask_image.data) {
									gen_params.mask_image = mask_image;
									imagesToCleanup.push_back(mask_image);
								}
							}
						}

						// Init Image component (for img2img)
						if (comp.contains("InitImage"))
						{
							nlohmann::json initImageData = comp["InitImage"];

							if (initImageData.contains("filePath") && !initImageData["filePath"].is_null() &&
								!initImageData["filePath"].get<std::string>().empty())
							{
								std::string initImagePath = initImageData["filePath"].get<std::string>();
								sd_image_t init_image = LoadImageToSDImage(initImagePath);
								if (init_image.data) {
									gen_params.init_image = init_image;
									imagesToCleanup.push_back(init_image);
								}
							}
						}

						// Note: VideoParams, HighNoiseSampler, and Chroma components are handled in ContextUtils
						// during context initialization. They don't affect individual image generation parameters.
					}
				}

				// Set up PhotoMaker ID images array
				if (!idImagesStorage.empty()) {
					pm_params.id_images_count = idImagesStorage.size();
					pm_params.id_images = idImagesStorage.data();
				}

				// Set up reference images array
				if (!refImagesStorage.empty()) {
					gen_params.ref_images_count = refImagesStorage.size();
					gen_params.ref_images = refImagesStorage.data();
				}

				// Set prompt strings
				gen_params.prompt = posPrompt.c_str();
				gen_params.negative_prompt = negPrompt.c_str();

				// Set VAE tiling parameters
				gen_params.vae_tiling_params = vae_tiling_params;

				// Set PhotoMaker parameters if available
				if (pm_params.id_embed_path != nullptr || pm_params.id_images_count > 0) {
					gen_params.pm_params = pm_params;
				}

				// Set ControlNet image and strength if available
				if (control_image.data != nullptr) {
					gen_params.control_image = control_image;
					gen_params.control_strength = control_strength;
					// Note: apply_start and apply_end are not in sd_img_gen_params_t
					// They would need to be set through a different mechanism
				}

				// Initialize SD context
				sd_context = InitializeStableDiffusionContext(metadata);
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
				CleanupResources(slg_layers, imagesToCleanup, result_images, sd_context);

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during txt2img: " << e.what() << std::endl;

				// Cleanup on error
				CleanupResources(slg_layers, imagesToCleanup, result_images, sd_context);

				return false;
			}
		}

	private:
		static void CleanupResources(int* slg_layers,
			std::vector<sd_image_t>& imagesToCleanup,
			sd_image_t* result_images,
			sd_ctx_t* sd_context) {
			// Cleanup SLG layers array if it was allocated
			if (slg_layers != nullptr)
			{
				delete[] slg_layers;
			}

			// Cleanup loaded images
			for (auto& img : imagesToCleanup) {
				FreeSDImage(img);
			}

			// Cleanup result images
			if (result_images)
			{
				free(result_images);
			}

			// Cleanup SD context
			if (sd_context)
			{
				free_sd_ctx(sd_context);
			}
		}
	};
} // namespace Utils