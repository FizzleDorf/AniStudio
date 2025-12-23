#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include <iostream>
#include <thread>

namespace Utils {

	// SD context initialization
	inline sd_ctx_t *InitializeStableDiffusionContext(const nlohmann::json &metadata)
	{
		try
		{
			// Use local strings to ensure proper lifetime
			std::string modelPath = "";
			std::string clipLPath = "";
			std::string clipGPath = "";
			std::string clipVisionPath = "";
			std::string t5xxlPath = "";
			std::string llmPath = "";
			std::string llmVisionPath = "";
			std::string diffusionModelPath = "";
			std::string highNoiseModelPath = "";
			std::string vaePath = "";
			std::string taesdPath = "";
			std::string controlnetPath = "";
			std::string photoMakerPath = "";
			std::string tensorTypeRules = "";  // Added for tensor_type_rules

			// Note: loraPath and embedPath commented out in ctx_params below

			// Get FilePaths instance for directory lookups
			FilePaths& filePaths = FilePaths::GetInstance();

			// Initialize context parameters with defaults
			sd_ctx_params_t ctx_params;

			// MANUALLY initialize all fields since sd_ctx_params_init might not set all
			ctx_params.model_path = nullptr;
			ctx_params.clip_l_path = nullptr;
			ctx_params.clip_g_path = nullptr;
			ctx_params.clip_vision_path = nullptr;
			ctx_params.t5xxl_path = nullptr;
			ctx_params.llm_path = nullptr;
			ctx_params.llm_vision_path = nullptr;
			ctx_params.diffusion_model_path = nullptr;
			ctx_params.high_noise_diffusion_model_path = nullptr;
			ctx_params.vae_path = nullptr;
			ctx_params.taesd_path = nullptr;
			ctx_params.control_net_path = nullptr;
			ctx_params.embeddings = nullptr;
			ctx_params.embedding_count = 0;
			ctx_params.photo_maker_path = nullptr;
			ctx_params.tensor_type_rules = nullptr;
			ctx_params.vae_decode_only = false;
			ctx_params.free_params_immediately = false;
			ctx_params.n_threads = std::thread::hardware_concurrency();
			ctx_params.wtype = SD_TYPE_F32;  // Default to F32
			ctx_params.rng_type = STD_DEFAULT_RNG;
			ctx_params.sampler_rng_type = STD_DEFAULT_RNG;
			ctx_params.prediction = EPS_PRED;  // Default prediction type
			ctx_params.lora_apply_mode = LORA_APPLY_AUTO;
			ctx_params.offload_params_to_cpu = false;
			ctx_params.keep_clip_on_cpu = false;
			ctx_params.keep_control_net_on_cpu = false;
			ctx_params.keep_vae_on_cpu = false;
			ctx_params.diffusion_flash_attn = false;
			ctx_params.tae_preview_only = false;
			ctx_params.diffusion_conv_direct = false;
			ctx_params.vae_conv_direct = false;
			ctx_params.circular_x = false;
			ctx_params.circular_y = false;
			ctx_params.force_sdxl_vae_conv_scale = false;
			ctx_params.chroma_use_dit_mask = false;
			ctx_params.chroma_use_t5_mask = false;
			ctx_params.chroma_t5_mask_pad = 0;
			ctx_params.flow_shift = 0.0f;

			// Extract parameters from components array in metadata
			if (metadata.contains("components") && metadata["components"].is_array())
			{
				for (const auto &comp : metadata["components"])
				{
					// Model component (Checkpoint)
					if (comp.contains("Checkpoint"))
					{
						auto model = comp["Checkpoint"];
						if (model.contains("modelPath") && !model["modelPath"].get<std::string>().empty())
							modelPath = model["modelPath"].get<std::string>();
						else if (model.contains("modelName") && !model["modelName"].get<std::string>().empty())
							modelPath = std::string(filePaths.GetPath("Checkpoint")) + "/" + model["modelName"].get<std::string>();

						// Debug output
						std::cout << "DEBUG: Model path set to: " << modelPath << std::endl;
					}

					// ClipL component
					if (comp.contains("ClipL"))
					{
						auto clipL = comp["ClipL"];
						if (clipL.contains("modelPath") && !clipL["modelPath"].get<std::string>().empty())
							clipLPath = clipL["modelPath"].get<std::string>();
						else if (clipL.contains("modelName") && !clipL["modelName"].get<std::string>().empty())
							clipLPath = std::string(filePaths.GetPath("Encoder")) + "/" + clipL["modelName"].get<std::string>();
					}

					// ClipG component
					if (comp.contains("ClipG"))
					{
						auto clipG = comp["ClipG"];
						if (clipG.contains("modelPath") && !clipG["modelPath"].get<std::string>().empty())
							clipGPath = clipG["modelPath"].get<std::string>();
						else if (clipG.contains("modelName") && !clipG["modelName"].get<std::string>().empty())
							clipGPath = std::string(filePaths.GetPath("Encoder")) + "/" + clipG["modelName"].get<std::string>();
					}

					// ClipVision component for I2V models
					if (comp.contains("ClipVision"))
					{
						auto clipVision = comp["ClipVision"];
						if (clipVision.contains("modelPath") && !clipVision["modelPath"].get<std::string>().empty())
							clipVisionPath = clipVision["modelPath"].get<std::string>();
						else if (clipVision.contains("modelName") && !clipVision["modelName"].get<std::string>().empty())
							clipVisionPath = std::string(filePaths.GetPath("Encoder")) + "/" + clipVision["modelName"].get<std::string>();
					}

					// T5XXL component
					if (comp.contains("T5XXL"))
					{
						auto t5xxl = comp["T5XXL"];
						if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].get<std::string>().empty())
							t5xxlPath = t5xxl["modelPath"].get<std::string>();
						else if (t5xxl.contains("modelName") && !t5xxl["modelName"].get<std::string>().empty())
							t5xxlPath = std::string(filePaths.GetPath("Encoder")) + "/" + t5xxl["modelName"].get<std::string>();
					}

					// llm text encoder component
					if (comp.contains("LLM"))
					{
						auto llm = comp["LLM"];
						if (llm.contains("modelPath") && !llm["modelPath"].get<std::string>().empty())
							llmPath = llm["modelPath"].get<std::string>();
						else if (llm.contains("modelName") && !llm["modelName"].get<std::string>().empty())
							llmPath = std::string(filePaths.GetPath("Encoder")) + "/" + llm["modelName"].get<std::string>();
					}

					// llmVision vision encoder component
					if (comp.contains("LLMVision"))
					{
						auto llmVision = comp["LLMVision"];
						if (llmVision.contains("modelPath") && !llmVision["modelPath"].get<std::string>().empty())
							llmVisionPath = llmVision["modelPath"].get<std::string>();
						else if (llmVision.contains("modelName") && !llmVision["modelName"].get<std::string>().empty())
							llmVisionPath = std::string(filePaths.GetPath("Encoder")) + "/" + llmVision["modelName"].get<std::string>();
					}

					// DiffusionModel component
					if (comp.contains("DiffusionModel"))
					{
						auto diffusion = comp["DiffusionModel"];
						if (diffusion.contains("modelPath") && !diffusion["modelPath"].get<std::string>().empty())
							diffusionModelPath = diffusion["modelPath"].get<std::string>();
						else if (diffusion.contains("modelName") && !diffusion["modelName"].get<std::string>().empty())
							diffusionModelPath = std::string(filePaths.GetPath("Unet")) + "/" + diffusion["modelName"].get<std::string>();
					}

					// HighNoiseDiffusionModel component (for video generation)
					if (comp.contains("HighNoiseDiffusionModel"))
					{
						auto highNoise = comp["HighNoiseDiffusionModel"];
						if (highNoise.contains("modelPath") && !highNoise["modelPath"].get<std::string>().empty())
							highNoiseModelPath = highNoise["modelPath"].get<std::string>();
						else if (highNoise.contains("modelName") && !highNoise["modelName"].get<std::string>().empty())
							highNoiseModelPath = std::string(filePaths.GetPath("Unet")) + "/" + highNoise["modelName"].get<std::string>();
					}

					// Vae component
					if (comp.contains("Vae"))
					{
						auto vae = comp["Vae"];
						if (vae.contains("modelPath") && !vae["modelPath"].get<std::string>().empty())
							vaePath = vae["modelPath"].get<std::string>();
						else if (vae.contains("modelName") && !vae["modelName"].get<std::string>().empty())
							vaePath = std::string(filePaths.GetPath("Vae")) + "/" + vae["modelName"].get<std::string>();
						if (vae.contains("vae_decode_only"))
							ctx_params.vae_decode_only = vae["vae_decode_only"].get<bool>();
						if (vae.contains("keep_vae_on_cpu"))
							ctx_params.keep_vae_on_cpu = vae["keep_vae_on_cpu"].get<bool>();
					}

					// Taesd component
					if (comp.contains("Taesd"))
					{
						auto taesd = comp["Taesd"];
						if (taesd.contains("modelPath") && !taesd["modelPath"].get<std::string>().empty())
							taesdPath = taesd["modelPath"].get<std::string>();
						else if (taesd.contains("modelName") && !taesd["modelName"].get<std::string>().empty())
							taesdPath = std::string(filePaths.GetPath("Vae")) + "/" + taesd["modelName"].get<std::string>();
					}

					// Controlnet component
					if (comp.contains("Controlnet"))
					{
						auto controlnet = comp["Controlnet"];
						if (controlnet.contains("modelPath") && !controlnet["modelPath"].get<std::string>().empty())
							controlnetPath = controlnet["modelPath"].get<std::string>();
						else if (controlnet.contains("modelName") && !controlnet["modelName"].get<std::string>().empty())
							controlnetPath = std::string(filePaths.GetPath("ControlNet")) + "/" + controlnet["modelName"].get<std::string>();

						if (controlnet.contains("keep_control_net_on_cpu"))
							ctx_params.keep_control_net_on_cpu = controlnet["keep_control_net_on_cpu"].get<bool>();
					}

					// Use PhotoMaker component to set photo_maker_path (this is what exists in your API)
					if (comp.contains("PhotoMaker") || comp.contains("StackedIdEmbed"))
					{
						auto pm = comp.contains("PhotoMaker") ? comp["PhotoMaker"] : comp["StackedIdEmbed"];
						if (pm.contains("modelPath") && !pm["modelPath"].get<std::string>().empty())
							photoMakerPath = pm["modelPath"].get<std::string>();
						else if (pm.contains("modelName") && !pm["modelName"].get<std::string>().empty())
							photoMakerPath = std::string(filePaths.GetPath("Embed")) + "/" + pm["modelName"].get<std::string>();
					}

					// Sampler component
					if (comp.contains("Sampler"))
					{
						auto sampler = comp["Sampler"];
						if (sampler.contains("n_threads"))
							ctx_params.n_threads = sampler["n_threads"].get<int>();
						if (sampler.contains("free_params_immediately"))
							ctx_params.free_params_immediately = sampler["free_params_immediately"].get<bool>();
						if (sampler.contains("offload_params_to_cpu"))
							ctx_params.offload_params_to_cpu = sampler["offload_params_to_cpu"].get<bool>();
						if (sampler.contains("keep_clip_on_cpu"))
							ctx_params.keep_clip_on_cpu = sampler["keep_clip_on_cpu"].get<bool>();
						if (sampler.contains("diffusion_flash_attn"))
							ctx_params.diffusion_flash_attn = sampler["diffusion_flash_attn"].get<bool>();
						if (sampler.contains("diffusion_conv_direct"))
							ctx_params.diffusion_conv_direct = sampler["diffusion_conv_direct"].get<bool>();
						if (sampler.contains("vae_conv_direct"))
							ctx_params.vae_conv_direct = sampler["vae_conv_direct"].get<bool>();
						if (sampler.contains("current_type_method"))
							ctx_params.wtype = static_cast<sd_type_t>(sampler["current_type_method"].get<int>());
						if (sampler.contains("current_prediction_type"))
							ctx_params.prediction = static_cast<prediction_t>(sampler["current_prediction_type"].get<int>());

						// Check for tensor type rules
						if (sampler.contains("tensor_type_rules") && !sampler["tensor_type_rules"].get<std::string>().empty())
							tensorTypeRules = sampler["tensor_type_rules"].get<std::string>();
					}

					// Latent Component
					if (comp.contains("Latent"))
					{
						auto latent = comp["Latent"];
						if (latent.contains("current_rng_type"))
							ctx_params.rng_type = static_cast<rng_type_t>(latent["current_rng_type"].get<int>());
						if (latent.contains("sampler_rng_type"))
							ctx_params.sampler_rng_type = static_cast<rng_type_t>(latent["sampler_rng_type"].get<int>());
					}

					// VideoParams component for flow_shift
					if (comp.contains("VideoParams"))
					{
						auto videoParams = comp["VideoParams"];
						if (videoParams.contains("flow_shift"))
							ctx_params.flow_shift = videoParams["flow_shift"].get<float>();
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
					}
				}
			}

			// Debug: Print all paths before creating context
			std::cout << "DEBUG: Creating SD context with paths:" << std::endl;
			std::cout << "  model_path: " << (modelPath.empty() ? "(empty)" : modelPath) << std::endl;
			std::cout << "  vae_path: " << (vaePath.empty() ? "(empty)" : vaePath) << std::endl;
			std::cout << "  clip_l_path: " << (clipLPath.empty() ? "(empty)" : clipLPath) << std::endl;
			std::cout << "  clip_g_path: " << (clipGPath.empty() ? "(empty)" : clipGPath) << std::endl;

			// Set paths in context parameters (only if not empty)
			ctx_params.model_path = modelPath.empty() ? nullptr : modelPath.c_str();
			ctx_params.clip_l_path = clipLPath.empty() ? nullptr : clipLPath.c_str();
			ctx_params.clip_g_path = clipGPath.empty() ? nullptr : clipGPath.c_str();
			ctx_params.clip_vision_path = clipVisionPath.empty() ? nullptr : clipVisionPath.c_str();
			ctx_params.t5xxl_path = t5xxlPath.empty() ? nullptr : t5xxlPath.c_str();
			ctx_params.llm_path = llmPath.empty() ? nullptr : llmPath.c_str();
			ctx_params.llm_vision_path = llmVisionPath.empty() ? nullptr : llmVisionPath.c_str();
			ctx_params.diffusion_model_path = diffusionModelPath.empty() ? nullptr : diffusionModelPath.c_str();
			ctx_params.high_noise_diffusion_model_path = highNoiseModelPath.empty() ? nullptr : highNoiseModelPath.c_str();
			ctx_params.vae_path = vaePath.empty() ? nullptr : vaePath.c_str();
			ctx_params.taesd_path = taesdPath.empty() ? nullptr : taesdPath.c_str();
			ctx_params.control_net_path = controlnetPath.empty() ? nullptr : controlnetPath.c_str();
			ctx_params.photo_maker_path = photoMakerPath.empty() ? nullptr : photoMakerPath.c_str();
			ctx_params.tensor_type_rules = tensorTypeRules.empty() ? nullptr : tensorTypeRules.c_str();

			// Note: The following are commented out in the original code
			// ctx_params.lora_model_dir = loraPath.c_str();
			// ctx_params.embedding_dir = embedPath.c_str();

			// Create context
			sd_ctx_t* ctx = new_sd_ctx(&ctx_params);

			if (!ctx) {
				std::cerr << "ERROR: Failed to create SD context!" << std::endl;
				return nullptr;
			}

			std::cout << "DEBUG: SD context created successfully!" << std::endl;
			return ctx;
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error initializing SD context: " << e.what() << std::endl;
			return nullptr;
		}
	}

} // namespace Utils