#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include <iostream>

namespace Utils {

	// SD context initialization - UPDATED for new API with proper string lifetime management
	inline sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata)
	{
		try
		{
			// Use local strings to ensure proper lifetime - these will be copied into the params
			std::string modelPath = "";
			std::string clipLPath = "";
			std::string clipGPath = "";
			std::string clipVisionPath = "";
			std::string t5xxlPath = "";
			std::string diffusionModelPath = "";
			std::string highNoiseModelPath = "";
			std::string vaePath = "";
			std::string taesdPath = "";
			std::string controlnetPath = "";
			std::string loraPath = "";
			std::string embedPath = "";
			std::string stackedIdEmbedPath = "";

			// Initialize context parameters with defaults
			sd_ctx_params_t ctx_params;
			sd_ctx_params_init(&ctx_params);

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
							modelPath = model["modelPath"].get<std::string>();
						else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
							modelPath = FilePaths::checkpointDir + "/" + model["modelName"].get<std::string>();
					}

					// ClipL component
					if (comp.contains("ClipL"))
					{
						auto clipL = comp["ClipL"];
						if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty())
							clipLPath = clipL["modelPath"].get<std::string>();
						else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
							clipLPath = FilePaths::encoderDir + "/" + clipL["modelName"].get<std::string>();
					}

					// ClipG component
					if (comp.contains("ClipG"))
					{
						auto clipG = comp["ClipG"];
						if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty())
							clipGPath = clipG["modelPath"].get<std::string>();
						else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
							clipGPath = FilePaths::encoderDir + "/" + clipG["modelName"].get<std::string>();
					}

					// ClipVision component for I2V models
					if (comp.contains("ClipVision"))
					{
						auto clipVision = comp["ClipVision"];
						if (clipVision.contains("modelPath") && !clipVision["modelPath"].get<std::string>().empty())
							clipVisionPath = clipVision["modelPath"].get<std::string>();
						else if (clipVision.contains("modelName") && !clipVision["modelName"].get<std::string>().empty())
							clipVisionPath = FilePaths::encoderDir + "/" + clipVision["modelName"].get<std::string>();
					}

					// T5XXL component
					if (comp.contains("T5XXL"))
					{
						auto t5xxl = comp["T5XXL"];
						if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].get<std::string>().empty())
							t5xxlPath = t5xxl["modelPath"].get<std::string>();
						else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
							t5xxlPath = FilePaths::encoderDir + "/" + t5xxl["modelName"].get<std::string>();
					}

					// DiffusionModel component
					if (comp.contains("DiffusionModel"))
					{
						auto diffusion = comp["DiffusionModel"];
						if (diffusion.contains("modelPath") && !diffusion["modelPath"].get<std::string>().empty())
							diffusionModelPath = diffusion["modelPath"].get<std::string>();
						else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
							diffusionModelPath = FilePaths::unetDir + "/" + diffusion["modelName"].get<std::string>();
					}

					// HighNoiseDiffusionModel component (for video generation)
					if (comp.contains("HighNoiseDiffusionModel"))
					{
						auto highNoise = comp["HighNoiseDiffusionModel"];
						if (highNoise.contains("modelPath") && !highNoise["modelPath"].get<std::string>().empty())
							highNoiseModelPath = highNoise["modelPath"].get<std::string>();
						else if (highNoise.contains("modelName") && !highNoise["modelName"].get<std::string>().empty())
							highNoiseModelPath = FilePaths::unetDir + "/" + highNoise["modelName"].get<std::string>();
					}

					// Vae component
					if (comp.contains("Vae"))
					{
						auto vae = comp["Vae"];
						if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty())
							vaePath = vae["modelPath"].get<std::string>();
						else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty())
							vaePath = FilePaths::vaeDir + "/" + vae["modelName"].get<std::string>();

						if (vae.contains("isTiled"))
							ctx_params.vae_tiling = vae["isTiled"].get<bool>();
						if (vae.contains("vae_decode_only"))
							ctx_params.vae_decode_only = vae["vae_decode_only"].get<bool>();
					}

					// Taesd component
					if (comp.contains("Taesd"))
					{
						auto taesd = comp["Taesd"];
						if (taesd.contains("modelPath") && !taesd["modelPath"].get<std::string>().empty())
							taesdPath = taesd["modelPath"].get<std::string>();
						else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
							taesdPath = FilePaths::vaeDir + "/" + taesd["modelName"].get<std::string>();
					}

					// Controlnet component
					if (comp.contains("Controlnet"))
					{
						auto controlnet = comp["Controlnet"];
						if (controlnet.contains("modelPath") && !controlnet["modelPath"].get<std::string>().empty())
							controlnetPath = controlnet["modelPath"].get<std::string>();
						else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
							controlnetPath = FilePaths::controlnetDir + "/" + controlnet["modelName"].get<std::string>();

						if (controlnet.contains("keep_control_net_on_cpu"))
							ctx_params.keep_control_net_on_cpu = controlnet["keep_control_net_on_cpu"].get<bool>();
					}

					// Lora component
					if (comp.contains("Lora"))
					{
						auto lora = comp["Lora"];
						if (lora.contains("modelPath") && !lora["modelPath"].get<std::string>().empty())
						{
							loraPath = lora["modelPath"].get<std::string>();
						}
						else if (lora.contains("modelName") && !lora["modelName"].get<std::string>().empty())
						{
							std::string modelName = lora["modelName"].get<std::string>();
							loraPath = FilePaths::loraDir + "/" + modelName;

							// Validation
							if (!std::filesystem::exists(loraPath)) {
								std::cout << "LoRA file not found: " << loraPath << ", skipping LoRA." << std::endl;
								loraPath = "";
							}
						}
					}

					// Embedding component
					if (comp.contains("Embedding"))
					{
						auto embed = comp["Embedding"];
						if (embed.contains("modelPath") && !embed["modelPath"].get<std::string>().empty())
							embedPath = embed["modelPath"].get<std::string>();
						else if (embed.contains("modelName") && !embed["modelName"].get<std::string>().empty())
							embedPath = FilePaths::embedDir + "/" + embed["modelName"].get<std::string>();
					}

					// Stacked ID Embedding component for PhotoMaker/Chroma support
					if (comp.contains("StackedIdEmbed"))
					{
						auto stackedEmbed = comp["StackedIdEmbed"];
						if (stackedEmbed.contains("modelPath") && !stackedEmbed["modelPath"].get<std::string>().empty())
							stackedIdEmbedPath = stackedEmbed["modelPath"].get<std::string>();
						else if (stackedEmbed.contains("modelName") && !stackedEmbed["modelName"].get<std::string>().empty())
							stackedIdEmbedPath = FilePaths::embedDir + "/" + stackedEmbed["modelName"].get<std::string>();
					}

					// Sampler component
					if (comp.contains("Sampler"))
					{
						auto sampler = comp["Sampler"];
						if (sampler.contains("n_threads"))
							ctx_params.n_threads = sampler["n_threads"].get<int>();
						if (sampler.contains("free_params_immediately"))
							ctx_params.free_params_immediately = sampler["free_params_immediately"].get<bool>();
						if (sampler.contains("keep_clip_on_cpu"))
							ctx_params.keep_clip_on_cpu = sampler["keep_clip_on_cpu"].get<bool>();
						if (sampler.contains("keep_vae_on_cpu"))
							ctx_params.keep_vae_on_cpu = sampler["keep_vae_on_cpu"].get<bool>();
						if (sampler.contains("offload_params_to_cpu"))
							ctx_params.offload_params_to_cpu = sampler["offload_params_to_cpu"].get<bool>();
						if (sampler.contains("diffusion_flash_attn"))
							ctx_params.diffusion_flash_attn = sampler["diffusion_flash_attn"].get<bool>();
						if (sampler.contains("offload_params_to_cpu"))
							ctx_params.offload_params_to_cpu = sampler["offload_params_to_cpu"].get<bool>();
						if (sampler.contains("diffusion_conv_direct"))
							ctx_params.diffusion_conv_direct = sampler["diffusion_conv_direct"].get<bool>();
						if (sampler.contains("vae_conv_direct"))
							ctx_params.vae_conv_direct = sampler["vae_conv_direct"].get<bool>();
						if (sampler.contains("current_type_method"))
							ctx_params.wtype = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						if (sampler.contains("current_rng_type"))
							ctx_params.rng_type = static_cast<rng_type_t>(sampler["current_rng_type"].get<int>());
					}

					// Chroma component for Chroma-specific settings
					if (comp.contains("Chroma"))
					{
						auto chroma = comp["Chroma"];
						if (chroma.contains("use_dit_mask"))
							ctx_params.chroma_use_dit_mask = chroma["use_dit_mask"].get<bool>();
						if (chroma.contains("use_t5_mask"))
							ctx_params.chroma_use_t5_mask = chroma["use_t5_mask"].get<bool>();
						if (chroma.contains("t5_mask_pad"))
							ctx_params.chroma_t5_mask_pad = chroma["t5_mask_pad"].get<int>();
						if (chroma.contains("flow_shift"))
							ctx_params.flow_shift = chroma["flow_shift"].get<float>();
					}
				}
			}

			// Set paths in context parameters - ALWAYS use c_str(), NEVER nullptr
			ctx_params.model_path = modelPath.c_str();
			ctx_params.clip_l_path = clipLPath.c_str();
			ctx_params.clip_g_path = clipGPath.c_str();
			ctx_params.clip_vision_path = clipVisionPath.c_str();
			ctx_params.t5xxl_path = t5xxlPath.c_str();
			ctx_params.diffusion_model_path = diffusionModelPath.c_str();
			ctx_params.high_noise_diffusion_model_path = highNoiseModelPath.c_str();
			ctx_params.vae_path = vaePath.c_str();
			ctx_params.taesd_path = taesdPath.c_str();
			ctx_params.control_net_path = controlnetPath.c_str();
			ctx_params.lora_model_dir = loraPath.c_str();
			ctx_params.embedding_dir = embedPath.c_str();
			ctx_params.stacked_id_embed_dir = stackedIdEmbedPath.c_str();

			// Log all paths for debugging
			std::cout << "Initializing SD context with the following parameters:" << std::endl;
			std::cout << "Model: " << ctx_params.model_path << std::endl;
			std::cout << "ClipL: " << ctx_params.clip_l_path << std::endl;
			std::cout << "ClipG: " << ctx_params.clip_g_path << std::endl;
			std::cout << "ClipVision: " << ctx_params.clip_vision_path << std::endl;
			std::cout << "T5XXL: " << ctx_params.t5xxl_path << std::endl;
			std::cout << "DiffusionModel: " << ctx_params.diffusion_model_path << std::endl;
			std::cout << "HighNoiseDiffusionModel: " << ctx_params.high_noise_diffusion_model_path << std::endl;
			std::cout << "Vae: " << ctx_params.vae_path << std::endl;
			std::cout << "Taesd: " << ctx_params.taesd_path << std::endl;
			std::cout << "Controlnet: " << ctx_params.control_net_path << std::endl;
			std::cout << "Lora: " << ctx_params.lora_model_dir << std::endl;
			std::cout << "Embedding: " << ctx_params.embedding_dir << std::endl;
			std::cout << "StackedIdEmbed: " << ctx_params.stacked_id_embed_dir << std::endl;

			// Chroma debug output
			std::cout << "Chroma DiT Mask: " << (ctx_params.chroma_use_dit_mask ? "true" : "false") << std::endl;
			std::cout << "Chroma T5 Mask: " << (ctx_params.chroma_use_t5_mask ? "true" : "false") << std::endl;
			std::cout << "Chroma T5 Mask Pad: " << ctx_params.chroma_t5_mask_pad << std::endl;

			// Initialize SD context using the NEW structured API
			return new_sd_ctx(&ctx_params);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error initializing SD context: " << e.what() << std::endl;
			return nullptr;
		}
	}

	// Helper function to initialize sample parameters
	inline void InitializeSampleParams(sd_sample_params_t &sample_params, const nlohmann::json &metadata)
	{
		sd_sample_params_init(&sample_params);

		// Extract sampling parameters from metadata
		if (metadata.contains("components") && metadata["components"].is_array())
		{
			for (const auto &comp : metadata["components"])
			{
				if (comp.contains("Sampler"))
				{
					auto sampler = comp["Sampler"];
					if (sampler.contains("current_sample_method"))
						sample_params.sample_method = static_cast<sample_method_t>(sampler["current_sample_method"].get<int>());
					if (sampler.contains("current_scheduler_method"))
						sample_params.scheduler = static_cast<scheduler_t>(sampler["current_scheduler_method"].get<int>());
					if (sampler.contains("steps"))
						sample_params.sample_steps = sampler["steps"].get<int>();
					if (sampler.contains("eta"))
						sample_params.eta = sampler["eta"].get<float>();
				}

				if (comp.contains("Guidance"))
				{
					auto guidance = comp["Guidance"];
					if (guidance.contains("guidance"))
						sample_params.guidance.txt_cfg = guidance["guidance"].get<float>();
					if (guidance.contains("img_cfg"))
						sample_params.guidance.img_cfg = guidance["img_cfg"].get<float>();
					if (guidance.contains("distilled_guidance"))
						sample_params.guidance.distilled_guidance = guidance["distilled_guidance"].get<float>();
				}

				// SLG parameters
				if (comp.contains("LayerSkip"))
				{
					auto layerSkip = comp["LayerSkip"];
					if (layerSkip.contains("slg_scale"))
						sample_params.guidance.slg.scale = layerSkip["slg_scale"].get<float>();
					if (layerSkip.contains("skip_layer_start"))
						sample_params.guidance.slg.layer_start = layerSkip["skip_layer_start"].get<float>();
					if (layerSkip.contains("skip_layer_end"))
						sample_params.guidance.slg.layer_end = layerSkip["skip_layer_end"].get<float>();

					// Handle skip layers array
					if (layerSkip.contains("skip_layers") && layerSkip["skip_layers"].is_array())
					{
						static std::vector<int> skip_layers;
						skip_layers.clear();
						for (const auto& layer : layerSkip["skip_layers"])
						{
							if (layer.is_number_integer())
								skip_layers.push_back(layer.get<int>());
						}
						if (!skip_layers.empty())
						{
							sample_params.guidance.slg.layers = skip_layers.data();
							sample_params.guidance.slg.layer_count = skip_layers.size();
						}
					}
				}
			}
		}
	}

} // namespace Utils