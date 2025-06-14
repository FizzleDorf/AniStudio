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
#include <stb_image.h>
#include <stb_image_write.h>
#include "pch.h"

namespace Utils {
	static std::random_device rd;
	static STDDefaultRNG rng;
	static bool initialized = false;

	class SDCPPUtils {
	public:

		// Generate a random seed using STDDefaultRNG from sdcpp
		// TODO: move this to a util
		static uint64_t generateRandomSeed() {
			// Seed the RNG with a random device if not already seeded
			if (!initialized) {
				rng.manual_seed(rd());
				initialized = true;
			}

			// Get random numbers
			std::vector<float> random_values = rng.randn(1);

			// Convert to a positive integer seed
			uint64_t seed = static_cast<uint64_t>(std::abs(random_values[0] * UINT32_MAX)) % INT32_MAX;
			return seed > 0 ? seed : 1; // Ensure seed is positive
		}

		// SD context initialization
		static sd_ctx_t* InitializeStableDiffusionContext(const nlohmann::json& metadata) {
			try {
				std::string modelPath = "", clipLPath = "", clipGPath = "", t5xxlPath = "";
				std::string diffusionModelPath = "", vaePath = "", taesdPath = "", controlnetPath = "";
				std::string loraPath = "", embedPath = "";
				bool vae_decode_only = false, isTiled = false, free_params_immediately = true;
				bool keep_vae_on_cpu = false;
				int n_threads = 4;
				sd_type_t type_method = SD_TYPE_F16;
				rng_type_t rng_type = STD_DEFAULT_RNG;
				schedule_t scheduler_method = DEFAULT;

				// Extract parameters from components array in metadata
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						// Model component
						if (comp.contains("Model")) {
							auto model = comp["Model"];
							if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty())
								modelPath = model["modelPath"];
							else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
								modelPath = FilePaths::checkpointDir + "/" + model["modelName"].get<std::string>();
						}

						// ClipL component
						if (comp.contains("ClipL")) {
							auto clipL = comp["ClipL"];
							if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty())
								clipLPath = clipL["modelPath"];
							else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
								clipLPath = FilePaths::encoderDir + "/" + clipL["modelName"].get<std::string>();
						}

						// ClipG component
						if (comp.contains("ClipG")) {
							auto clipG = comp["ClipG"];
							if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty())
								clipGPath = clipG["modelPath"];
							else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
								clipGPath = FilePaths::encoderDir + "/" + clipG["modelName"].get<std::string>();
						}

						// T5XXL component
						if (comp.contains("T5XXL")) {
							auto t5xxl = comp["T5XXL"];
							if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].get<std::string>().empty())
								t5xxlPath = t5xxl["modelPath"];
							else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
								t5xxlPath = FilePaths::encoderDir + "/" + t5xxl["modelName"].get<std::string>();
						}

						// DiffusionModel component
						if (comp.contains("DiffusionModel")) {
							auto diffusion = comp["DiffusionModel"];
							if (diffusion.contains("modelPath") && !diffusion["modelPath"].get<std::string>().empty())
								diffusionModelPath = diffusion["modelPath"];
							else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
								diffusionModelPath = FilePaths::unetDir + "/" + diffusion["modelName"].get<std::string>();
						}

						// Vae component
						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty())
								vaePath = vae["modelPath"];
							else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty())
								vaePath = FilePaths::vaeDir + "/" + vae["modelName"].get<std::string>();

							if (vae.contains("isTiled"))
								isTiled = vae["isTiled"];
							if (vae.contains("keep_vae_on_cpu"))
								keep_vae_on_cpu = vae["keep_vae_on_cpu"];
							if (vae.contains("vae_decode_only"))
								vae_decode_only = vae["vae_decode_only"];
						}

						// Taesd component
						if (comp.contains("Taesd")) {
							auto taesd = comp["Taesd"];
							if (taesd.contains("modelPath") && !taesd["modelPath"].get<std::string>().empty())
								taesdPath = taesd["modelPath"];
							else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
								taesdPath = FilePaths::vaeDir + "/" + taesd["modelName"].get<std::string>();
						}

						// Controlnet component
						if (comp.contains("Controlnet")) {
							auto controlnet = comp["Controlnet"];
							if (controlnet.contains("modelPath") && !controlnet["modelPath"].get<std::string>().empty())
								controlnetPath = controlnet["modelPath"];
							else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
								controlnetPath = FilePaths::controlnetDir + "/" + controlnet["modelName"].get<std::string>();
						}

						// Lora component
						if (comp.contains("Lora")) {
							auto lora = comp["Lora"];
							if (lora.contains("modelPath") && !lora["modelPath"].get<std::string>().empty()) {
								loraPath = lora["modelPath"];
							}
							else if (lora.contains("modelName") && !lora["modelName"].get<std::string>().empty()) {
								std::string modelName = lora["modelName"].get<std::string>();
								loraPath = FilePaths::loraDir + "/" + modelName;
							}
							else {
								// Default to lora directory if no specific model is specified
								loraPath = FilePaths::loraDir;
							}
						}
						else {
							// Always set loraPath to the directory if the component doesn't exist
							loraPath = FilePaths::loraDir;
						}

						// Embedding component
						if (comp.contains("EmbeddingComponent")) {
							auto embed = comp["EmbeddingComponent"];
							if (embed.contains("modelPath") && !embed["modelPath"].get<std::string>().empty())
								embedPath = embed["modelPath"];
							else if (embed.contains("modelName") && !embed["modelName"].get<std::string>().empty())
								embedPath = FilePaths::embedDir + "/" + embed["modelName"].get<std::string>();
						}

						// Sampler component
						if (comp.contains("Sampler")) {
							auto sampler = comp["Sampler"];
							if (sampler.contains("n_threads"))
								n_threads = sampler["n_threads"];
							if (sampler.contains("free_params_immediately"))
								free_params_immediately = sampler["free_params_immediately"];
							if (sampler.contains("current_type_method"))
								type_method = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
							if (sampler.contains("current_rng_type"))
								rng_type = static_cast<rng_type_t>(sampler["current_rng_type"].get<int>());
							if (sampler.contains("current_scheduler_method"))
								scheduler_method = static_cast<schedule_t>(sampler["current_scheduler_method"].get<int>());
						}
					}
				}

				// Log all paths for debugging
				std::cout << "Initializing SD context with the following paths:" << std::endl;
				std::cout << "Model: " << modelPath << std::endl;
				std::cout << "ClipL: " << clipLPath << std::endl;
				std::cout << "ClipG: " << clipGPath << std::endl;
				std::cout << "T5XXL: " << t5xxlPath << std::endl;
				std::cout << "DiffusionModel: " << diffusionModelPath << std::endl;
				std::cout << "Vae: " << vaePath << std::endl;
				std::cout << "Taesd: " << taesdPath << std::endl;
				std::cout << "Controlnet: " << controlnetPath << std::endl;
				std::cout << "Lora: " << loraPath << std::endl;
				std::cout << "Embedding: " << embedPath << std::endl;

				// Initialize SD context with parsed metadata
				return new_sd_ctx(
					modelPath.c_str(),
					clipLPath.c_str(),
					clipGPath.c_str(),
					t5xxlPath.c_str(),
					diffusionModelPath.c_str(),
					vaePath.c_str(),
					taesdPath.c_str(),
					controlnetPath.c_str(),
					loraPath.c_str(),
					embedPath.c_str(),
					"",  // placeholder_token_text
					vae_decode_only,
					isTiled,
					free_params_immediately,
					n_threads,
					type_method,
					rng_type,
					scheduler_method,
					true,  // shift_text_decoder
					false, // debug_clip_pos
					keep_vae_on_cpu,
					false  // debug_extract_shifts
				);
			}
			catch (const std::exception& e) {
				std::cerr << "Error initializing SD context: " << e.what() << std::endl;
				return nullptr;
			}
		}

		// Generate image based on metadata parameters
		static sd_image_t* GenerateImage(sd_ctx_t* context, const nlohmann::json& metadata) {
			std::string posPrompt = "", negPrompt = "";
			float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
			int latentWidth = 512, latentHeight = 512, steps = 20, seed = -1, batchSize = 1;
			sample_method_t sample_method = EULER;
			int* skipLayers = nullptr;
			size_t skipLayersCount = 0;
			float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;

			// Extract parameters from components array in metadata
			if (metadata.contains("components") && metadata["components"].is_array()) {
				for (const auto& comp : metadata["components"]) {
					// Prompt component
					if (comp.contains("Prompt")) {
						auto prompt = comp["Prompt"];
						if (prompt.contains("posPrompt"))
							posPrompt = prompt["posPrompt"];
						if (prompt.contains("negPrompt"))
							negPrompt = prompt["negPrompt"];
					}

					// ClipSkip component
					if (comp.contains("ClipSkip")) {
						auto clipSkipComp = comp["ClipSkip"];
						if (clipSkipComp.contains("clipSkip"))
							clipSkip = clipSkipComp["clipSkip"];
					}

					// Sampler component
					if (comp.contains("Sampler")) {
						auto sampler = comp["Sampler"];
						if (sampler.contains("cfg"))
							cfg = sampler["cfg"];
						if (sampler.contains("steps"))
							steps = sampler["steps"];
						if (sampler.contains("seed"))
							seed = sampler["seed"];
						if (sampler.contains("current_sample_method"))
							sample_method = static_cast<sample_method_t>(sampler["current_sample_method"].get<int>());
					}

					// Guidance component
					if (comp.contains("Guidance")) {
						auto guidanceComp = comp["Guidance"];
						if (guidanceComp.contains("guidance"))
							guidance = guidanceComp["guidance"];
						if (guidanceComp.contains("eta"))
							eta = guidanceComp["eta"];
					}

					// Latent component
					if (comp.contains("Latent")) {
						auto latent = comp["Latent"];
						if (latent.contains("latentWidth"))
							latentWidth = latent["latentWidth"];
						if (latent.contains("latentHeight"))
							latentHeight = latent["latentHeight"];
						if (latent.contains("batchSize"))
							batchSize = latent["batchSize"];
					}

					// Skip layers component
					if (comp.contains("LayerSkip")) {
						auto layerSkip = comp["LayerSkip"];
						if (layerSkip.contains("skip_layers"))
							skipLayers = reinterpret_cast<int*>(layerSkip["skip_layers"].get<intptr_t>());
						if (layerSkip.contains("skip_layers_count"))
							skipLayersCount = layerSkip["skip_layers_count"];
						if (layerSkip.contains("slg_scale"))
							slgScale = layerSkip["slg_scale"];
						if (layerSkip.contains("skip_layer_start"))
							skipLayerStart = layerSkip["skip_layer_start"];
						if (layerSkip.contains("skip_layer_end"))
							skipLayerEnd = layerSkip["skip_layer_end"];
					}
				}
			}

			// Ensure valid seed
			if (seed < 0) {
				seed = static_cast<int>(generateRandomSeed());
				std::cout << "Generated random seed: " << seed << std::endl;
			}

			// Call the Stable Diffusion txt2img function with extracted parameters
			return txt2img(
				context,
				posPrompt.c_str(),
				negPrompt.c_str(),
				clipSkip,
				cfg,
				guidance,
				eta,
				latentWidth,
				latentHeight,
				sample_method,
				steps,
				seed,
				batchSize,
				nullptr,  // control_image
				0.0f,     // control_strength
				0.0f,     // style_strength
				false,    // normalize_input
				"",       // input_id_images_path
				skipLayers,
				skipLayersCount,
				slgScale,
				skipLayerStart,
				skipLayerEnd
			);
		}

		// Implementations of core methods for generation

		static bool RunInference(const nlohmann::json& metadata, std::string fullPath) {
			sd_ctx_t* sd_context = nullptr;
			sd_image_t* image = nullptr;

			try {
				// Initialize Stable Diffusion context
				std::cout << "Initializing SD context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context) {
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Generate image
				image = GenerateImage(sd_context, metadata);
				if (!image) {
					throw std::runtime_error("Failed to generate image!");
				}

				// Save the generated image
				SaveImage(image->data, image->width, image->height, image->channel, metadata, fullPath);

				// Cleanup
				if (image) {
					free(image);
					image = nullptr;
				}

				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during inference: " << e.what() << std::endl;

				// Clean up resources
				if (image) {
					free(image);
					image = nullptr;
				}

				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return false;
			}
		}
		static bool ConvertToGGUF(const nlohmann::json& metadata) {
			try {
				std::string inputPath, vaePath;
				sd_type_t type = SD_TYPE_F16;

				// Extract model paths from metadata
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						if (comp.contains("Model")) {
							auto model = comp["Model"];
							if (model.contains("modelPath"))
								inputPath = model["modelPath"];
						}

						if (comp.contains("Vae")) {
							auto vae = comp["Vae"];
							if (vae.contains("modelPath"))
								vaePath = vae["modelPath"];
						}

						if (comp.contains("Sampler")) {
							auto sampler = comp["Sampler"];
							if (sampler.contains("current_type_method"))
								type = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						}
					}
				}

				// Validate input path
				if (inputPath.empty()) {
					throw std::runtime_error("Input model path is empty");
				}

				// Create output path with type suffix
				std::filesystem::path inPath(inputPath);
				std::string outPath = inPath.parent_path().string() + "/" +
					inPath.stem().string() + "_" +
					std::string(type_method_items[type]) + ".gguf";

				// Perform conversion
				bool result;
				if (vaePath.empty()) {
					// Convert without VAE
					result = convert(inputPath.c_str(), nullptr, outPath.c_str(), type);
				}
				else {
					// Convert with VAE
					result = convert(inputPath.c_str(), vaePath.c_str(), outPath.c_str(), type);
				}

				if (!result) {
					throw std::runtime_error("Failed to convert Model: " + inputPath);
				}

				std::cout << "Successfully converted model to: " << outPath << std::endl;
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during conversion: " << e.what() << std::endl;
				return false;
			}
		}

		static bool RunImg2Img(const nlohmann::json& metadata, std::string fullPath) {
			sd_ctx_t* sd_context = nullptr;
			unsigned char* inputData = nullptr;
			unsigned char* maskData = nullptr;
			unsigned char* emptyMaskData = nullptr;
			sd_image_t* result_image = nullptr;

			try {
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
				int* skipLayers = nullptr;
				size_t skipLayersCount = 0;
				float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;

				// Debug logging for metadata
				std::cout << "Img2Img metadata:" << std::endl;
				std::cout << metadata.dump(2) << std::endl;

				// Parse metadata to extract parameters
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						// Input image path
						if (comp.contains("InputImage")) {
							nlohmann::json inputImageData = comp["InputImage"];

							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null()
								&& !inputImageData["filePath"].get<std::string>().empty()) {
								inputImagePath = inputImageData["filePath"].get<std::string>();
								std::cout << "Found input path: " << inputImagePath << std::endl;
							}
						}

						// Mask image path (additional for img2img)
						if (comp.contains("MaskImage")) {
							nlohmann::json maskImageData = comp["MaskImage"];

							if (maskImageData.contains("filePath") && !maskImageData["filePath"].is_null()
								&& !maskImageData["filePath"].get<std::string>().empty()) {
								maskImagePath = maskImageData["filePath"].get<std::string>();
								std::cout << "Found mask path: " << maskImagePath << std::endl;
							}
						}

						// Output settings
						if (comp.contains("OutputImage")) {
							nlohmann::json outputImageData = comp["OutputImage"];

							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null()
								&& !outputImageData["filePath"].get<std::string>().empty()) {
								outputPath = outputImageData["filePath"].get<std::string>();
								std::cout << "Found output path: " << outputPath << std::endl;
							}

							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null()
								&& !outputImageData["fileName"].get<std::string>().empty()) {
								outputFilename = outputImageData["fileName"].get<std::string>();
								std::cout << "Found output filename: " << outputFilename << std::endl;
							}
						}

						// Prompt component
						if (comp.contains("Prompt")) {
							nlohmann::json promptData = comp["Prompt"];

							if (promptData.contains("posPrompt") && !promptData["posPrompt"].is_null())
								posPrompt = promptData["posPrompt"].get<std::string>();
							if (promptData.contains("negPrompt") && !promptData["negPrompt"].is_null())
								negPrompt = promptData["negPrompt"].get<std::string>();
						}

						// ClipSkip component
						if (comp.contains("ClipSkip")) {
							nlohmann::json clipSkipData = comp["ClipSkip"];

							if (clipSkipData.contains("clipSkip") && !clipSkipData["clipSkip"].is_null())
								clipSkip = clipSkipData["clipSkip"].get<float>();
						}

						// Sampler component
						if (comp.contains("Sampler")) {
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
						if (comp.contains("Guidance")) {
							nlohmann::json guidanceData = comp["Guidance"];

							if (guidanceData.contains("guidance") && !guidanceData["guidance"].is_null())
								guidance = guidanceData["guidance"].get<float>();
							if (guidanceData.contains("eta") && !guidanceData["eta"].is_null())
								eta = guidanceData["eta"].get<float>();
						}

						// Latent component
						if (comp.contains("Latent")) {
							nlohmann::json latentData = comp["Latent"];

							if (latentData.contains("latentWidth") && !latentData["latentWidth"].is_null())
								latentWidth = latentData["latentWidth"].get<int>();
							if (latentData.contains("latentHeight") && !latentData["latentHeight"].is_null())
								latentHeight = latentData["latentHeight"].get<int>();
							if (latentData.contains("batchSize") && !latentData["batchSize"].is_null())
								batchSize = latentData["batchSize"].get<int>();
						}

						// Layer Skip component
						if (comp.contains("LayerSkip")) {
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
				if (inputImagePath.empty()) {
					throw std::runtime_error("Input image path is empty!");
				}

				// Check if input image file exists
				if (!std::filesystem::exists(inputImagePath)) {
					throw std::runtime_error("Input image file does not exist: " + inputImagePath);
				}

				// Load input image
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;

				// Force 3 channels (RGB) for consistency with stable-diffusion
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 3);
				if (!inputData) {
					std::string error = std::string("Failed to load input image: ") + inputImagePath + " - " +
						(stbi_failure_reason() ? stbi_failure_reason() : "unknown reason");
					throw std::runtime_error(error);
				}

				// Force channels to 3 since we requested RGB
				inputChannels = 3;

				std::cout << "Input image loaded successfully: " << inputWidth << "x" << inputHeight
					<< " with " << inputChannels << " channels (forced RGB)" << std::endl;

				// Validate image dimensions
				if (inputWidth <= 0 || inputHeight <= 0) {
					throw std::runtime_error("Invalid input image dimensions: " + std::to_string(inputWidth) + "x" + std::to_string(inputHeight));
				}

				// Create output path - SAME AS UPSCALING
				std::filesystem::path outputDir(outputPath);
				std::filesystem::path outputFile(outputFilename);
				std::string uniqueFilePath = Utils::PngMetadata::CreateUniqueFilename(
					outputFile.string(), outputDir.string());

				// Initialize SD context - SAME AS UPSCALING PATTERN
				std::cout << "Initializing Stable Diffusion context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context) {
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Create input image struct - FORCE RGB FORMAT
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					3,  // FORCE 3 channels (RGB)
					inputData
				};

				// Initialize mask image struct - IMPROVED MASK HANDLING
				sd_image_t mask_image = { 0 };

				if (maskImagePath.empty() || !std::filesystem::exists(maskImagePath)) {
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
				else {
					// Load mask from file - FORCE GRAYSCALE
					int maskWidth, maskHeight, maskChannels;
					std::cout << "Loading mask image from: " << maskImagePath << std::endl;

					// FORCE 1 channel (grayscale) for mask
					maskData = stbi_load(maskImagePath.c_str(), &maskWidth, &maskHeight, &maskChannels, 1);

					if (!maskData) {
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
					else {
						// Validate mask dimensions match input image
						if (maskWidth != inputWidth || maskHeight != inputHeight) {
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
						else {
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
				if (seed < 0) {
					seed = static_cast<int>(generateRandomSeed());
					std::cout << "Generated random seed: " << seed << std::endl;
				}

				// Validate parameters before calling img2img
				std::cout << "img2img parameters:" << std::endl;
				std::cout << "  Positive prompt: \"" << posPrompt << "\"" << std::endl;
				std::cout << "  Negative prompt: \"" << negPrompt << "\"" << std::endl;
				std::cout << "  Clip skip: " << clipSkip << std::endl;
				std::cout << "  CFG: " << cfg << std::endl;
				std::cout << "  Guidance: " << guidance << std::endl;
				std::cout << "  Steps: " << steps << std::endl;
				std::cout << "  Seed: " << seed << std::endl;
				std::cout << "  Denoise: " << denoiseStrength << std::endl;
				std::cout << "  Dimensions: " << latentWidth << "x" << latentHeight << std::endl;

				// Perform img2img - using the same pattern as upscaling
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
					0.0f,    // control_strength
					0.0f,    // style_ratio
					false,   // normalize_input
					"",      // input_id_images_path
					skipLayers,
					skipLayersCount,
					slgScale,
					skipLayerStart,
					skipLayerEnd
				);

				if (!result_image) {
					throw std::runtime_error("img2img failed - no output image produced");
				}

				if (!result_image->data) {
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

				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during img2img: " << e.what() << std::endl;

				// Clean up resources
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

				if (sd_context) {
					free_sd_ctx(sd_context);
					sd_context = nullptr;
				}

				return false;
			}
		}

		static bool RunUpscaling(const nlohmann::json& metadata, std::string fullPath) {
			upscaler_ctx_t* upscaler_context = nullptr;
			unsigned char* inputData = nullptr;

			try {
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
				if (metadata.contains("components") && metadata["components"].is_array()) {
					for (const auto& comp : metadata["components"]) {
						// Input image path
						if (comp.contains("InputImage")) {
							nlohmann::json inputImageData;
							inputImageData = comp["InputImage"];

							if (inputImageData.contains("filePath") && !inputImageData["filePath"].is_null()
								&& !inputImageData["filePath"].get<std::string>().empty()) {
								inputImagePath = inputImageData["filePath"].get<std::string>();
								std::cout << "Found input path: " << inputImagePath << std::endl;
							}
						}

						// Output settings
						if (comp.contains("OutputImage")) {
							nlohmann::json outputImageData;
							outputImageData = comp["OutputImage"];

							if (outputImageData.contains("filePath") && !outputImageData["filePath"].is_null()
								&& !outputImageData["filePath"].get<std::string>().empty()) {
								outputPath = outputImageData["filePath"].get<std::string>();
								std::cout << "Found output path: " << outputPath << std::endl;
							}

							if (outputImageData.contains("fileName") && !outputImageData["fileName"].is_null()
								&& !outputImageData["fileName"].get<std::string>().empty()) {
								outputFilename = outputImageData["fileName"].get<std::string>();
								std::cout << "Found output filename: " << outputFilename << std::endl;
							}
						}

						// Esrgan component
						if (comp.contains("Esrgan")) {
							nlohmann::json esrganData;
							esrganData = comp["Esrgan"];


							if (esrganData.contains("modelPath") && !esrganData["modelPath"].is_null()
								&& !esrganData["modelPath"].get<std::string>().empty()) {
								modelPath = esrganData["modelPath"];
							}
							else if (esrganData.contains("modelName") && !esrganData["modelName"].is_null()
								&& !esrganData["modelName"].get<std::string>().empty()) {
								std::string modelName = esrganData["modelName"].get<std::string>();
								modelPath = (std::filesystem::path(Utils::FilePaths::upscaleDir) / modelName).string();
							}

							if (esrganData.contains("upscaleFactor"))
								upscaleFactor = esrganData["upscaleFactor"];

							if (esrganData.contains("preserveAspectRatio"))
								preserveAspectRatio = esrganData["preserveAspectRatio"];
						}

						// Sampler component
						if (comp.contains("Sampler")) {
							nlohmann::json samplerData;
							samplerData = comp["Sampler"];

							if (samplerData.contains("n_threads"))
								n_threads = samplerData["n_threads"];
						}
					}
				}

				// Validate parameters
				if (inputImagePath.empty()) {
					throw std::runtime_error("Input image path is empty!");
				}

				if (modelPath.empty()) {
					throw std::runtime_error("ESRGAN model path is empty!");
				}

				// Load input image
				int inputWidth, inputHeight, inputChannels;
				std::cout << "Loading input image from: " << inputImagePath << std::endl;
				inputData = stbi_load(inputImagePath.c_str(), &inputWidth, &inputHeight, &inputChannels, 0);
				if (!inputData) {
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
				if (!upscaler_context) {
					throw std::runtime_error("Failed to initialize upscaler context!");
				}

				// Create input image struct
				sd_image_t input_image = {
					static_cast<uint32_t>(inputWidth),
					static_cast<uint32_t>(inputHeight),
					static_cast<uint32_t>(inputChannels),
					inputData
				};

				// Perform upscaling
				sd_image_t upscaled_image = upscale(upscaler_context, input_image, upscaleFactor);
				if (!upscaled_image.data) {
					throw std::runtime_error("Upscaling failed - no output image produced");
				}

				std::cout << "Upscaling successful, saving output to: " << uniqueFilePath << std::endl;

				// Update metadata with correct output path before saving
				nlohmann::json updatedMetadata = metadata;
				for (auto& comp : updatedMetadata["components"]) {
					if (comp.contains("OutputImage")) {
						if (comp["OutputImage"].contains("OutputImage")) {
							comp["OutputImage"]["OutputImage"]["filePath"] = uniqueFilePath;
						}
						else {
							comp["OutputImage"]["filePath"] = uniqueFilePath;
						}
					}
				}

				// Save the upscaled image
				SaveImage(upscaled_image.data, upscaled_image.width, upscaled_image.height,
					upscaled_image.channel, metadata, fullPath);
				std::this_thread::sleep_for(std::chrono::milliseconds(50));


				// Cleanup resources
				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				// Free the upscaled image if needed
				if (upscaled_image.data) {
					free(upscaled_image.data);
				}

				// Cleanup upscaler context
				free_upscaler_ctx(upscaler_context);
				upscaler_context = nullptr;

				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "Exception during upscaling: " << e.what() << std::endl;

				// Clean up resources
				if (inputData) {
					stbi_image_free(inputData);
					inputData = nullptr;
				}

				if (upscaler_context) {
					free_upscaler_ctx(upscaler_context);
					upscaler_context = nullptr;
				}

				return false;
			}
		}

		static void SaveImage(const unsigned char* data, int width, int height, int channels,
			const nlohmann::json& metadata, std::string fullPath) {
			try {

				// Save file
				if (!Utils::ImageUtils::SaveImage(
					fullPath, width, height, channels, data)) {
					std::cerr << "Failed to save image: " << fullPath << std::endl;
				}

				// Save metadata to the PNG
				Utils::PngMetadata::WriteMetadataToPNG(fullPath, metadata);

				std::cout << "Image saved successfully: \"" << fullPath << "\"" << std::endl;
			}
			catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Error creating directory: " << e.what() << '\n';
			}
			catch (const std::exception& e) {
				std::cerr << "Exception in SaveImage: " << e.what() << '\n';
			}
		}
	};
} // namespace ECS