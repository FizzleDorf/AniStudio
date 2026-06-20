#pragma once
#include "stable-diffusion.h"
#include "DiffusionCallbackUtils.hpp"

namespace Utils {
	// Parse context creation parameters from metadata
	static bool ParseContextParams(const nlohmann::json& metadata, sd_ctx_params_t& ctx_params,
		std::string& modelPath, std::string& vaePath, std::string& clipLPath,
		std::string& clipGPath, std::string& clipVisionPath, std::string& t5xxlPath,
		std::string& llmPath, std::string& llmVisionPath, std::string& diffusionModelPath,
		std::string& highNoiseModelPath, std::string& taesdPath, std::string& controlnetPath,
		std::string& photoMakerPath, std::string& tensorTypeRules) {

		try {
			sd_ctx_params_init(&ctx_params);

			if (!metadata.contains("components") || !metadata["components"].is_array()) {
				return false;
			}

			for (const auto& comp : metadata["components"]) {
				// Checkpoint
				if (comp.contains("Checkpoint")) {
					auto model = comp["Checkpoint"];
					if (model.contains("modelPath") && !model["modelPath"].is_null())
						modelPath = model["modelPath"].get<std::string>();
					else if (model.contains("modelName") && !model["modelName"].is_null())
						modelPath = FilePathService::GetPath("Checkpoint") + "/" + model["modelName"].get<std::string>();

					if (!modelPath.empty()) {
						ctx_params.model_path = modelPath.c_str();
					}
				}

				// VAE
				if (comp.contains("Vae")) {
					auto vae = comp["Vae"];
					if (vae.contains("modelPath") && !vae["modelPath"].is_null())
						vaePath = vae["modelPath"].get<std::string>();
					else if (vae.contains("modelName") && !vae["modelName"].is_null())
						vaePath = FilePathService::GetPath("Vae") + "/" + vae["modelName"].get<std::string>();

					if (!vaePath.empty()) {
						ctx_params.vae_path = vaePath.c_str();
					}
				}

				// CLIP models
				if (comp.contains("ClipL")) {
					auto clipL = comp["ClipL"];
					if (clipL.contains("modelPath") && !clipL["modelPath"].is_null())
						clipLPath = clipL["modelPath"].get<std::string>();
					else if (clipL.contains("modelName") && !clipL["modelName"].is_null())
						clipLPath = FilePathService::GetPath("Encoder") + "/" + clipL["modelName"].get<std::string>();

					if (!clipLPath.empty()) {
						ctx_params.clip_l_path = clipLPath.c_str();
					}
				}

				if (comp.contains("ClipG")) {
					auto clipG = comp["ClipG"];
					if (clipG.contains("modelPath") && !clipG["modelPath"].is_null())
						clipGPath = clipG["modelPath"].get<std::string>();
					else if (clipG.contains("modelName") && !clipG["modelName"].is_null())
						clipGPath = FilePathService::GetPath("Encoder") + "/" + clipG["modelName"].get<std::string>();

					if (!clipGPath.empty()) {
						ctx_params.clip_g_path = clipGPath.c_str();
					}
				}

				if (comp.contains("ClipVision")) {
					auto clipVision = comp["ClipVision"];
					if (clipVision.contains("modelPath") && !clipVision["modelPath"].is_null())
						clipVisionPath = clipVision["modelPath"].get<std::string>();
					else if (clipVision.contains("modelName") && !clipVision["modelName"].is_null())
						clipVisionPath = FilePathService::GetPath("Encoder") + "/" + clipVision["modelName"].get<std::string>();

					if (!clipVisionPath.empty()) {
						ctx_params.clip_vision_path = clipVisionPath.c_str();
					}
				}

				if (comp.contains("T5XXL")) {
					auto t5xxl = comp["T5XXL"];
					if (t5xxl.contains("modelPath") && !t5xxl["modelPath"].is_null())
						t5xxlPath = t5xxl["modelPath"].get<std::string>();
					else if (t5xxl.contains("modelName") && !t5xxl["modelName"].is_null())
						t5xxlPath = FilePathService::GetPath("Encoder") + "/" + t5xxl["modelName"].get<std::string>();

					if (!t5xxlPath.empty()) {
						ctx_params.t5xxl_path = t5xxlPath.c_str();
					}
				}

				if (comp.contains("LLM")) {
					auto llm = comp["LLM"];
					if (llm.contains("modelPath") && !llm["modelPath"].is_null())
						llmPath = llm["modelPath"].get<std::string>();
					else if (llm.contains("modelName") && !llm["modelName"].is_null())
						llmPath = FilePathService::GetPath("Encoder") + "/" + llm["modelName"].get<std::string>();

					if (!llmPath.empty()) {
						ctx_params.llm_path = llmPath.c_str();
					}
				}

				if (comp.contains("LLMVision")) {
					auto llmVision = comp["LLMVision"];
					if (llmVision.contains("modelPath") && !llmVision["modelPath"].is_null())
						llmVisionPath = llmVision["modelPath"].get<std::string>();
					else if (llmVision.contains("modelName") && !llmVision["modelName"].is_null())
						llmVisionPath = FilePathService::GetPath("Encoder") + "/" + llmVision["modelName"].get<std::string>();

					if (!llmVisionPath.empty()) {
						ctx_params.llm_vision_path = llmVisionPath.c_str();
					}
				}

				if (comp.contains("DiffusionModel")) {
					auto diffusion = comp["DiffusionModel"];
					if (diffusion.contains("modelPath") && !diffusion["modelPath"].is_null())
						diffusionModelPath = diffusion["modelPath"].get<std::string>();
					else if (diffusion.contains("modelName") && !diffusion["modelName"].is_null())
						diffusionModelPath = FilePathService::GetPath("Unet") + "/" + diffusion["modelName"].get<std::string>();

					if (!diffusionModelPath.empty()) {
						ctx_params.diffusion_model_path = diffusionModelPath.c_str();
					}
				}

				if (comp.contains("HighNoiseDiffusionModel")) {
					auto highNoise = comp["HighNoiseDiffusionModel"];
					if (highNoise.contains("modelPath") && !highNoise["modelPath"].is_null())
						highNoiseModelPath = highNoise["modelPath"].get<std::string>();
					else if (highNoise.contains("modelName") && !highNoise["modelName"].is_null())
						highNoiseModelPath = FilePathService::GetPath("Unet") + "/" + highNoise["modelName"].get<std::string>();

					if (!highNoiseModelPath.empty()) {
						ctx_params.high_noise_diffusion_model_path = highNoiseModelPath.c_str();
					}
				}

				if (comp.contains("Taesd")) {
					auto taesd = comp["Taesd"];
					if (taesd.contains("modelPath") && !taesd["modelPath"].is_null())
						taesdPath = taesd["modelPath"].get<std::string>();
					else if (taesd.contains("modelName") && !taesd["modelName"].is_null())
						taesdPath = FilePathService::GetPath("Vae") + "/" + taesd["modelName"].get<std::string>();

					if (!taesdPath.empty()) {
						ctx_params.taesd_path = taesdPath.c_str();
					}
				}

				if (comp.contains("Controlnet")) {
					auto controlnet = comp["Controlnet"];
					if (controlnet.contains("modelPath") && !controlnet["modelPath"].is_null())
						controlnetPath = controlnet["modelPath"].get<std::string>();
					else if (controlnet.contains("modelName") && !controlnet["modelName"].is_null())
						controlnetPath = FilePathService::GetPath("ControlNet") + "/" + controlnet["modelName"].get<std::string>();

					if (!controlnetPath.empty()) {
						ctx_params.control_net_path = controlnetPath.c_str();
					}
				}

				if (comp.contains("PhotoMaker") || comp.contains("StackedIdEmbed")) {
					auto pm = comp.contains("PhotoMaker") ? comp["PhotoMaker"] : comp["StackedIdEmbed"];
					if (pm.contains("modelPath") && !pm["modelPath"].is_null())
						photoMakerPath = pm["modelPath"].get<std::string>();
					else if (pm.contains("modelName") && !pm["modelName"].is_null())
						photoMakerPath = FilePathService::GetPath("Embed") + "/" + pm["modelName"].get<std::string>();

					if (!photoMakerPath.empty()) {
						ctx_params.photo_maker_path = photoMakerPath.c_str();
					}
				}

				if (comp.contains("Sampler")) {
					auto sampler = comp["Sampler"];
					if (sampler.contains("n_threads"))
						ctx_params.n_threads = sampler["n_threads"].get<int>();
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

					if (sampler.contains("tensor_type_rules") && !sampler["tensor_type_rules"].is_null()) {
						tensorTypeRules = sampler["tensor_type_rules"].get<std::string>();
						if (!tensorTypeRules.empty()) {
							ctx_params.tensor_type_rules = tensorTypeRules.c_str();
						}
					}
				}
			}
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Error parsing context params: " << e.what() << std::endl;
			return false;
		}
	}
}