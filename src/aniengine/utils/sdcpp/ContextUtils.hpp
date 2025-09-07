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
#include "pch.h"
#include "RngUtils.hpp"
#include "ContextUtils.hpp" 
#include "SaveUtils.hpp"
#include <iostream>

namespace Utils {

	// SD context initialization
	inline sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata)
	{
		try
		{
			std::string modelPath = "", clipLPath = "", clipGPath = "", t5xxlPath = "";
			std::string diffusionModelPath = "", vaePath = "", taesdPath = "", controlnetPath = "";
			std::string loraPath = "", embedPath = "", stackedIdEmbedPath = "";
			bool vae_decode_only = false, isTiled = false, free_params_immediately = true;
			bool keep_clip_on_cpu = true, keep_control_net_cpu = false, keep_vae_on_cpu = false;
			bool diffusion_flash_attn = false;
			bool chroma_use_dit_mask = false, chroma_use_t5_mask = false;
			int chroma_t5_mask_pad = 0;
			int n_threads = 4;
			sd_type_t type_method = SD_TYPE_F16;
			rng_type_t rng_type = STD_DEFAULT_RNG;
			schedule_t scheduler_method = DEFAULT;

			// Extract parameters from components array in metadata
			if (metadata.contains("components") && metadata["components"].is_array())
			{
				for (const auto &comp : metadata["components"])
				{
					// Model component
					if (comp.contains("Model"))
					{
						auto model = comp["Model"];
						if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty())
							modelPath = model["modelPath"];
						else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
							modelPath = FilePaths::checkpointDir + "/" + model["modelName"].get<std::string>();
					}

					// ClipL component
					if (comp.contains("ClipL"))
					{
						auto clipL = comp["ClipL"];
						if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty())
							clipLPath = clipL["modelPath"];
						else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
							clipLPath = FilePaths::encoderDir + "/" + clipL["modelName"].get<std::string>();
					}

					// ClipG component
					if (comp.contains("ClipG"))
					{
						auto clipG = comp["ClipG"];
						if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty())
							clipGPath = clipG["modelPath"];
						else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
							clipGPath = FilePaths::encoderDir + "/" + clipG["modelName"].get<std::string>();
					}

					// T5XXL component
					if (comp.contains("T5XXL"))
					{
						auto t5xxl = comp["T5XXL"];
						if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].get<std::string>().empty())
							t5xxlPath = t5xxl["modelPath"];
						else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
							t5xxlPath = FilePaths::encoderDir + "/" + t5xxl["modelName"].get<std::string>();
					}

					// DiffusionModel component
					if (comp.contains("DiffusionModel"))
					{
						auto diffusion = comp["DiffusionModel"];
						if (diffusion.contains("modelPath") && !diffusion["modelPath"].get<std::string>().empty())
							diffusionModelPath = diffusion["modelPath"];
						else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
							diffusionModelPath = FilePaths::unetDir + "/" + diffusion["modelName"].get<std::string>();
					}

					// Vae component
					if (comp.contains("Vae"))
					{
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
					if (comp.contains("Taesd"))
					{
						auto taesd = comp["Taesd"];
						if (taesd.contains("modelPath") && !taesd["modelPath"].get<std::string>().empty())
							taesdPath = taesd["modelPath"];
						else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
							taesdPath = FilePaths::vaeDir + "/" + taesd["modelName"].get<std::string>();
					}

					// Controlnet component
					if (comp.contains("Controlnet"))
					{
						auto controlnet = comp["Controlnet"];
						if (controlnet.contains("modelPath") && !controlnet["modelPath"].get<std::string>().empty())
							controlnetPath = controlnet["modelPath"];
						else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
							controlnetPath = FilePaths::controlnetDir + "/" + controlnet["modelName"].get<std::string>();
					}

					// Lora component
					if (comp.contains("Lora"))
					{
						auto lora = comp["Lora"];
						if (lora.contains("modelPath") && !lora["modelPath"].get<std::string>().empty())
						{
							loraPath = lora["modelPath"];
						}
						else if (lora.contains("modelName") && !lora["modelName"].get<std::string>().empty())
						{
							std::string modelName = lora["modelName"].get<std::string>();
							loraPath = FilePaths::loraDir + "/" + modelName;
						}
						else
						{
							// Default to lora directory if no specific model is specified
							loraPath = FilePaths::loraDir;
						}
					}
					else
					{
						// Always set loraPath to the directory if the component doesn't exist
						loraPath = FilePaths::loraDir;
					}

					// Embedding component
					if (comp.contains("Embedding"))
					{
						auto embed = comp["Embedding"];
						if (embed.contains("modelPath") && !embed["modelPath"].get<std::string>().empty())
							embedPath = embed["modelPath"];
						else if (embed.contains("modelName") && !embed["modelName"].get<std::string>().empty())
							embedPath = FilePaths::embedDir + "/" + embed["modelName"].get<std::string>();
					}

					// Stacked ID Embedding component for PhotoMaker/Chroma support
					if (comp.contains("StackedIdEmbed"))
					{
						auto stackedEmbed = comp["StackedIdEmbed"];
						if (stackedEmbed.contains("modelPath") && !stackedEmbed["modelPath"].get<std::string>().empty())
							stackedIdEmbedPath = stackedEmbed["modelPath"];
						else if (stackedEmbed.contains("modelName") && !stackedEmbed["modelName"].get<std::string>().empty())
							stackedIdEmbedPath = FilePaths::embedDir + "/" + stackedEmbed["modelName"].get<std::string>();
					}

					// Sampler component
					if (comp.contains("Sampler"))
					{
						auto sampler = comp["Sampler"];
						if (sampler.contains("n_threads"))
							n_threads = sampler["n_threads"];
						if (sampler.contains("free_params_immediately"))
							free_params_immediately = sampler["free_params_immediately"];
						if (sampler.contains("keep_clip_on_cpu"))
							keep_clip_on_cpu = sampler["keep_clip_on_cpu"];
						if (sampler.contains("keep_control_net_cpu"))
							keep_control_net_cpu = sampler["keep_control_net_cpu"];
						if (sampler.contains("diffusion_flash_attn"))
							diffusion_flash_attn = sampler["diffusion_flash_attn"];
						if (sampler.contains("current_type_method"))
							type_method = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						if (sampler.contains("current_rng_type"))
							rng_type = static_cast<rng_type_t>(sampler["current_rng_type"].get<int>());
						if (sampler.contains("current_scheduler_method"))
							scheduler_method = static_cast<schedule_t>(sampler["current_scheduler_method"].get<int>());
					}

					// Chroma component for Chroma-specific settings
					if (comp.contains("Chroma"))
					{
						auto chroma = comp["Chroma"];
						if (chroma.contains("use_dit_mask"))
							chroma_use_dit_mask = chroma["use_dit_mask"];
						if (chroma.contains("use_t5_mask"))
							chroma_use_t5_mask = chroma["use_t5_mask"];
						if (chroma.contains("t5_mask_pad"))
							chroma_t5_mask_pad = chroma["t5_mask_pad"];
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
			std::cout << "StackedIdEmbed: " << stackedIdEmbedPath << std::endl;

			// Chroma debug output
			std::cout << "Chroma DiT Mask: " << (chroma_use_dit_mask ? "true" : "false") << std::endl;
			std::cout << "Chroma T5 Mask: " << (chroma_use_t5_mask ? "true" : "false") << std::endl;
			std::cout << "Chroma T5 Mask Pad: " << chroma_t5_mask_pad << std::endl;

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
				stackedIdEmbedPath.c_str(),  // stacked_id_embed_dir_c_str
				vae_decode_only,
				isTiled,  // vae_tiling
				free_params_immediately,
				n_threads,
				type_method,  // wtype
				rng_type,
				scheduler_method,  // s (schedule)
				keep_clip_on_cpu,
				keep_control_net_cpu,
				keep_vae_on_cpu,
				diffusion_flash_attn,
				chroma_use_dit_mask,
				chroma_use_t5_mask,
				chroma_t5_mask_pad
			);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error initializing SD context: " << e.what() << std::endl;
			return nullptr;
		}
	}

} // namespace Utils