#pragma once

#include "stable-diffusion.h"
#include "PngMetadataUtils.hpp"
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include "FilePathService.hpp"
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>
#include <filesystem>

namespace Utils
{
	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);

	class Img2Img
	{
	public:
		static bool RunImg2Img(const nlohmann::json &metadata, const std::string &fullPath, sd_ctx_t *sd_context = nullptr)
		{
			bool contextProvided = (sd_context != nullptr);
			unsigned char *inputData = nullptr;
			unsigned char *maskData = nullptr;
			unsigned char *emptyMaskData = nullptr;
			sd_image_t *result_image = nullptr;

			// Arrays to track additional images that need cleanup
			std::vector<sd_image_t> imagesToCleanup;
			std::vector<sd_image_t> idImagesStorage;
			std::vector<sd_image_t> refImagesStorage;
			int* slg_layers = nullptr;

			try
			{
				std::string inputImagePath = "";
				std::string maskImagePath = "";
				std::string outputPath = Utils::FilePathService::GetPath("DefaultProject");
				std::string outputFilename = "img2img_output.png";
				std::string posPrompt = "";
				std::string negPrompt = "";
				int latentWidth = 0;
				int latentHeight = 0;

				// VAE tiling parameters
				sd_tiling_params_t vae_tiling_params = { false, 64, 64, 0.0f, 64.0f, 64.0f };

				// PhotoMaker parameters
				sd_pm_params_t pm_params = { nullptr, 0, nullptr, 0.0f };

				// ControlNet image
				sd_image_t control_image = { 0, 0, 0, nullptr };
				float control_strength = 0.0f;

				sd_img_gen_params_t gen_params;
				sd_img_gen_params_init(&gen_params);

				if (metadata.contains("components") && metadata["components"].is_array())
				{
					for (const auto &comp : metadata["components"])
					{
						if (comp.contains("InputImage"))
						{
							nlohmann::json inputImageData = comp["InputImage"];
							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null())
								inputImagePath = inputImageData["filePath"].get<std::string>();
						}

						if (comp.contains("MaskImage"))
						{
							nlohmann::json maskImageData = comp["MaskImage"];
							if (maskImageData.contains("filePath") && !maskImageData["filePath"].is_null())
								maskImagePath = maskImageData["filePath"].get<std::string>();
						}

						if (comp.contains("OutputImage"))
						{
							nlohmann::json outputImageData = comp["OutputImage"];
							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null())
								outputPath = outputImageData["filePath"].get<std::string>();
							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null())
								outputFilename = outputImageData["fileName"].get<std::string>();
						}

						if (comp.contains("Prompt"))
						{
							nlohmann::json promptData = comp["Prompt"];
							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								posPrompt = promptData["posPrompt"].get<std::string>();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								negPrompt = promptData["negPrompt"].get<std::string>();
						}

						if (comp.contains("ClipSkip"))
						{
							nlohmann::json clipSkipData = comp["ClipSkip"];
							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								gen_params.clip_skip = clipSkipData["clipSkip"].get<int>();
						}

						if (comp.contains("Sampler"))
						{
							nlohmann::json samplerData = comp["Sampler"];
							if (samplerData.contains("seed") && !samplerData["seed"].is_null())
								gen_params.seed = static_cast<int64_t>(samplerData["seed"].get<int>());
							if (samplerData.contains("denoise") && !samplerData["denoise"].is_null())
								gen_params.strength = samplerData["denoise"].get<float>();
							if (samplerData.contains("batchSize") && !samplerData["batchSize"].is_null())
								gen_params.batch_count = samplerData["batchSize"].get<int>();
							if (samplerData.contains("steps") && !samplerData["steps"].is_null())
								gen_params.sample_params.sample_steps = samplerData["steps"].get<int>();
							if (samplerData.contains("eta") && !samplerData["eta"].is_null())
								gen_params.sample_params.eta = samplerData["eta"].get<float>();
							if (samplerData.contains("current_sample_method") && !samplerData["current_sample_method"].is_null())
								gen_params.sample_params.sample_method = static_cast<sample_method_t>(samplerData["current_sample_method"].get<int>());
							if (samplerData.contains("current_scheduler_method") && !samplerData["current_scheduler_method"].is_null())
								gen_params.sample_params.scheduler = static_cast<scheduler_t>(samplerData["current_scheduler_method"].get<int>());
						}

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

						if (comp.contains("Latent"))
						{
							nlohmann::json latentData = comp["Latent"];
							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								latentWidth = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								latentHeight = latentData["latentHeight"].get<int>();
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

						// ControlNet component
						if (comp.contains("Controlnet"))
						{
							nlohmann::json controlNetData = comp["Controlnet"];
							if (controlNetData.contains("cnStrength") && !controlNetData["cnStrength"].is_null())
							{
								control_strength = controlNetData["cnStrength"].get<float>();
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
					}
				}

				gen_params.prompt = posPrompt.c_str();
				gen_params.negative_prompt = negPrompt.c_str();
				gen_params.control_image = control_image;
				gen_params.control_strength = control_strength;

				// Set VAE tiling parameters
				gen_params.vae_tiling_params = vae_tiling_params;

				// Set up PhotoMaker ID images array
				if (!idImagesStorage.empty()) {
					pm_params.id_images_count = idImagesStorage.size();
					pm_params.id_images = idImagesStorage.data();
					gen_params.pm_params = pm_params;
				}

				// Set up reference images array
				if (!refImagesStorage.empty()) {
					gen_params.ref_images_count = refImagesStorage.size();
					gen_params.ref_images = refImagesStorage.data();
				}

				if (inputImagePath.empty()) {
					throw std::runtime_error("Input image path is empty!");
				}

				if (!std::filesystem::exists(inputImagePath)) {
					throw std::runtime_error("Input image file not found: " + inputImagePath);
				}

				std::cout << "=== Img2Img Debug ===" << std::endl;
				std::cout << "Input image path: " << inputImagePath << std::endl;

				int inputWidth, inputHeight, inputChannels;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData) {
					throw std::runtime_error("Failed to load input image: " + inputImagePath);
				}
				inputChannels = 3;

				std::cout << "Loaded image dimensions: " << inputWidth << "x" << inputHeight << std::endl;

				if (latentWidth <= 0 || latentHeight <= 0) {
					latentWidth = inputWidth;
					latentHeight = inputHeight;
				}

				if (latentWidth != inputWidth || latentHeight != inputHeight) {
					std::cout << "WARNING: Latent dimensions (" << latentWidth << "x" << latentHeight
						<< ") don't match input image (" << inputWidth << "x" << inputHeight << ")" << std::endl;
					std::cout << "Auto-adjusting latent to match input image..." << std::endl;
					latentWidth = inputWidth;
					latentHeight = inputHeight;
				}

				gen_params.width = static_cast<uint32_t>(latentWidth);
				gen_params.height = static_cast<uint32_t>(latentHeight);

				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3,
					inputData };

				gen_params.init_image = input_image;

				if (maskImagePath.empty() || !std::filesystem::exists(maskImagePath))
				{
					size_t maskSize = inputWidth * inputHeight;
					emptyMaskData = new unsigned char[maskSize];
					std::memset(emptyMaskData, 255, maskSize);

					gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
					gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
					gen_params.mask_image.channel = 1;
					gen_params.mask_image.data = emptyMaskData;
				}
				else
				{
					int maskWidth, maskHeight, maskChannels;
					maskData = stbi_load(maskImagePath.c_str(), &maskWidth, &maskHeight, &maskChannels, 1);

					if (!maskData)
					{
						size_t maskSize = inputWidth * inputHeight;
						emptyMaskData = new unsigned char[maskSize];
						std::memset(emptyMaskData, 255, maskSize);

						gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
						gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
						gen_params.mask_image.channel = 1;
						gen_params.mask_image.data = emptyMaskData;
					}
					else
					{
						if (maskWidth != inputWidth || maskHeight != inputHeight)
						{
							stbi_image_free(maskData);
							maskData = nullptr;

							size_t maskSize = inputWidth * inputHeight;
							emptyMaskData = new unsigned char[maskSize];
							std::memset(emptyMaskData, 255, maskSize);

							gen_params.mask_image.width = static_cast<uint32_t>(inputWidth);
							gen_params.mask_image.height = static_cast<uint32_t>(inputHeight);
							gen_params.mask_image.channel = 1;
							gen_params.mask_image.data = emptyMaskData;
						}
						else
						{
							gen_params.mask_image.width = static_cast<uint32_t>(maskWidth);
							gen_params.mask_image.height = static_cast<uint32_t>(maskHeight);
							gen_params.mask_image.channel = 1;
							gen_params.mask_image.data = maskData;
						}
					}
				}

				std::cout << "Final settings:" << std::endl;
				std::cout << "  - Input image: " << inputWidth << "x" << inputHeight << std::endl;
				std::cout << "  - Target size: " << gen_params.width << "x" << gen_params.height << std::endl;
				std::cout << "  - Strength: " << gen_params.strength << std::endl;
				std::cout << "=====================" << std::endl;

				// Get SD context if not provided
				if (!contextProvided) {
					sd_context = SDContextManager::GetOrCreateContext(metadata);
				}

				if (!sd_context) {
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				if (gen_params.seed < 0) {
					gen_params.seed = static_cast<int64_t>(generateRandomSeed());
				}

				result_image = generate_image(sd_context, &gen_params);
				if (!result_image || !result_image->data) {
					throw std::runtime_error("generate_image failed");
				}

				SaveImage(result_image->data, result_image->width, result_image->height,
					result_image->channel, metadata, fullPath);

				// Cleanup
				CleanupResources(inputData, maskData, emptyMaskData, result_image,
					slg_layers, imagesToCleanup);

				// Release context back to cache if we acquired it
				if (!contextProvided) {
					SDContextManager::ReleaseContext(sd_context);
				}

				return true;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception during img2img: " << e.what() << std::endl;

				// Cleanup on error
				CleanupResources(inputData, maskData, emptyMaskData, result_image,
					slg_layers, imagesToCleanup);

				// Release context back to cache if we acquired it
				if (!contextProvided && sd_context) {
					SDContextManager::ReleaseContext(sd_context);
				}

				return false;
			}
		}

	private:
		static void CleanupResources(unsigned char* inputData,
			unsigned char* maskData,
			unsigned char* emptyMaskData,
			sd_image_t* result_image,
			int* slg_layers,
			std::vector<sd_image_t>& imagesToCleanup) {

			// Cleanup main image data
			if (inputData) {
				stbi_image_free(inputData);
				inputData = nullptr;
			}
			if (maskData) {
				stbi_image_free(maskData);
				maskData = nullptr;
			}
			if (emptyMaskData) {
				delete[] emptyMaskData;
				emptyMaskData = nullptr;
			}
			if (result_image) {
				free(result_image);
				result_image = nullptr;
			}

			// Cleanup SLG layers array if it was allocated
			if (slg_layers != nullptr)
			{
				delete[] slg_layers;
			}

			// Cleanup additional loaded images
			for (auto& img : imagesToCleanup) {
				FreeSDImage(img);
			}
		}

		// Helper function to load image from file to sd_image_t
		static sd_image_t LoadImageToSDImage(const std::string& filePath) {
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
				result.data = data;
			}

			return result;
		}

		// Helper to free sd_image_t data
		static void FreeSDImage(sd_image_t& img) {
			if (img.data) {
				stbi_image_free(img.data);
				img.data = nullptr;
			}
		}
	};
}