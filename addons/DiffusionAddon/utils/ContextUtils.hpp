#pragma once

#include "stable-diffusion.h"
#include "pch.h"
#include <iostream>

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
			std::string qwen2vlPath = "";
			std::string qwen2vlVisionPath = "";
			std::string diffusionModelPath = "";
			std::string highNoiseModelPath = "";
			std::string vaePath = "";
			std::string taesdPath = "";
			std::string controlnetPath = "";
			std::string loraPath = "";
			std::string embedPath = "";
			std::string photoMakerPath = "";

			// Get FilePaths instance for directory lookups
			FilePaths& filePaths = FilePaths::GetInstance();

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
							modelPath = std::string(filePaths.GetPath("Checkpoint")) + "/" + model["modelName"].get<std::string>();
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

					// Qwen2VL text encoder component
					if (comp.contains("Qwen2VL"))
					{
						auto qwen2vl = comp["Qwen2VL"];
						if (qwen2vl.contains("modelPath") && !qwen2vl["modelPath"].get<std::string>().empty())
							qwen2vlPath = qwen2vl["modelPath"].get<std::string>();
						else if (qwen2vl.contains("modelName") && !qwen2vl["modelName"].get<std::string>().empty())
							qwen2vlPath = std::string(filePaths.GetPath("Encoder")) + "/" + qwen2vl["modelName"].get<std::string>();
					}

					// Qwen2VL vision encoder component
					if (comp.contains("Qwen2VLVision"))
					{
						auto qwen2vlVision = comp["Qwen2VLVision"];
						if (qwen2vlVision.contains("modelPath") && !qwen2vlVision["modelPath"].get<std::string>().empty())
							qwen2vlVisionPath = qwen2vlVision["modelPath"].get<std::string>();
						else if (qwen2vlVision.contains("modelName") && !qwen2vlVision["modelName"].get<std::string>().empty())
							qwen2vlVisionPath = std::string(filePaths.GetPath("Encoder")) + "/" + qwen2vlVision["modelName"].get<std::string>();
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

					// Lora component
					if (comp.contains("Lora"))
					{
						auto lora = comp["Lora"];
						if (lora.contains("modelPath") && !lora["modelPath"].get<std::string>().empty())
						{
							loraPath = lora["modelPath"].get<std::string>();

							if (loraPath.empty()) {
								loraPath = filePaths.GetPath("Lora");
							}
						}
					}

					// Embedding component
					if (comp.contains("Embedding"))
					{
						auto embed = comp["Embedding"];
						if (embed.contains("modelPath") && !embed["modelPath"].get<std::string>().empty())
						{
							embedPath = embed["modelPath"].get<std::string>();

							if (embedPath.empty()) {
								embedPath = filePaths.GetPath("Embed");
							}
						}
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
					}

					// Latent Component
					if (comp.contains("Latent"))
					{
						auto latent = comp["Latent"];
						if (latent.contains("current_rng_type"))
							ctx_params.rng_type = static_cast<rng_type_t>(latent["current_rng_type"].get<int>());
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

			// Set paths in context parameters
			ctx_params.model_path = modelPath.c_str();
			ctx_params.clip_l_path = clipLPath.c_str();
			ctx_params.clip_g_path = clipGPath.c_str();
			ctx_params.clip_vision_path = clipVisionPath.c_str();
			ctx_params.t5xxl_path = t5xxlPath.c_str();
			ctx_params.qwen2vl_path = qwen2vlPath.c_str();
			ctx_params.qwen2vl_vision_path = qwen2vlVisionPath.c_str();
			ctx_params.diffusion_model_path = diffusionModelPath.c_str();
			ctx_params.high_noise_diffusion_model_path = highNoiseModelPath.c_str();
			ctx_params.vae_path = vaePath.c_str();
			ctx_params.taesd_path = taesdPath.c_str();
			ctx_params.control_net_path = controlnetPath.c_str();
			ctx_params.lora_model_dir = loraPath.c_str();
			ctx_params.embedding_dir = embedPath.c_str();
			ctx_params.photo_maker_path = photoMakerPath.c_str();


			sd_ctx_t* ctx = new_sd_ctx(&ctx_params);

			return ctx;
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error initializing SD context: " << e.what() << std::endl;
			return nullptr;
		}
	}

} // namespace Utils