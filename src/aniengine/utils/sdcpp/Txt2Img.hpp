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
	extern std::random_device rd;
	extern STDDefaultRNG rng;
	extern bool initialized;

	uint64_t generateRandomSeed();
	void SaveImage(const unsigned char *data, int width, int height, int channels,
		const nlohmann::json &metadata, const std::string &fullPath);
	sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata);

	class Txt2Img
	{
	public:
		// Generate image based on metadata parameters
		static sd_image_t *GenerateImage(sd_ctx_t *context, const nlohmann::json &metadata)
		{
			std::string posPrompt = "", negPrompt = "";
			float clipSkip = 2.0f, cfg = 7.0f, guidance = 2.0f, eta = 0.0f;
			int latentWidth = 512, latentHeight = 512, steps = 20, seed = -1, batchSize = 1;
			sample_method_t sample_method = EULER;
			int *skipLayers = nullptr;
			size_t skipLayersCount = 0;
			float slgScale = 0.0f, skipLayerStart = 0.0f, skipLayerEnd = 1.0f;

			// Extract parameters from components array in metadata
			if (metadata.contains("components") && metadata["components"].is_array())
			{
				for (const auto &comp : metadata["components"])
				{
					// Prompt component
					if (comp.contains("Prompt"))
					{
						auto prompt = comp["Prompt"];
						if (prompt.contains("posPrompt"))
							posPrompt = prompt["posPrompt"];
						if (prompt.contains("negPrompt"))
							negPrompt = prompt["negPrompt"];
					}

					// ClipSkip component
					if (comp.contains("ClipSkip"))
					{
						auto clipSkipComp = comp["ClipSkip"];
						if (clipSkipComp.contains("clipSkip"))
							clipSkip = clipSkipComp["clipSkip"];
					}

					// Sampler component
					if (comp.contains("Sampler"))
					{
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
					if (comp.contains("Guidance"))
					{
						auto guidanceComp = comp["Guidance"];
						if (guidanceComp.contains("guidance"))
							guidance = guidanceComp["guidance"];
						if (guidanceComp.contains("eta"))
							eta = guidanceComp["eta"];
					}

					// Latent component
					if (comp.contains("Latent"))
					{
						auto latent = comp["Latent"];
						if (latent.contains("latentWidth"))
							latentWidth = latent["latentWidth"];
						if (latent.contains("latentHeight"))
							latentHeight = latent["latentHeight"];
						if (latent.contains("batchSize"))
							batchSize = latent["batchSize"];
					}

					// Skip layers component
					if (comp.contains("LayerSkip"))
					{
						auto layerSkip = comp["LayerSkip"];
						if (layerSkip.contains("skip_layers"))
							skipLayers = reinterpret_cast<int *>(layerSkip["skip_layers"].get<intptr_t>());
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
			if (seed < 0)
			{
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
				nullptr, // control_image
				0.0f,	 // control_strength
				0.0f,	 // style_strength
				false,	 // normalize_input
				"",		 // input_id_images_path
				skipLayers,
				skipLayersCount,
				slgScale,
				skipLayerStart,
				skipLayerEnd);
		}

		// Main inference function
		static bool RunInference(const nlohmann::json &metadata, std::string fullPath)
		{
			sd_ctx_t *sd_context = nullptr;
			sd_image_t *image = nullptr;

			try
			{
				// Initialize Stable Diffusion context
				std::cout << "Initializing SD context..." << std::endl;
				sd_context = InitializeStableDiffusionContext(metadata);
				if (!sd_context)
				{
					throw std::runtime_error("Failed to initialize Stable Diffusion context!");
				}

				// Generate image
				image = GenerateImage(sd_context, metadata);
				if (!image)
				{
					throw std::runtime_error("Failed to generate image!");
				}

				// Save the generated image
				SaveImage(image->data, image->width, image->height, image->channel, metadata, fullPath);

				// Cleanup
				if (image)
				{
					free(image);
					image = nullptr;
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
				std::cerr << "Exception during inference: " << e.what() << std::endl;

				// Clean up resources
				if (image)
				{
					free(image);
					image = nullptr;
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