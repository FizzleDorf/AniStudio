#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>
#include <vector>

namespace ECS {

	struct SamplerComponent : public ECS::BaseComponent {
		SamplerComponent() {
			compName = "Sampler";
			compCategory = "Sampling";

			schema = {
				{"title", "Sampler Settings"},
				{"type", "object"},
				{"propertyOrder", {
					"current_sample_method", "current_scheduler_method", "current_type_method",
					"current_prediction_type",
					"seed", "steps", "eta", "denoise", "n_threads", "free_params_immediately",
					"offload_params_to_cpu", "keep_clip_on_cpu", "keep_control_net_on_cpu",
					"diffusion_flash_attn", "diffusion_conv_direct", "vae_conv_direct",
					"force_sdxl_vae_conv_scale", "shifted_timestep",
					"lora_apply_mode", "enable_mmap", "qwen_image_zero_cond_t",
					"max_vram", "stream_layers", "eager_load", "backend", "params_backend", "rpc_servers",
					"extra_sample_args"
				}},
				{"properties", {
					{"current_sample_method", {
						{"type", "integer"},
						{"title", "Sampler"},
						{"description", "The sampling method used for denoising. Euler is fast and stable, DPM++ methods provide higher quality at the cost of speed."},
						{"ui:widget", "combo"},
						{"items", sample_method_items},
						{"itemCount", sample_method_item_count}
					}},
					{"current_scheduler_method", {
						{"type", "integer"},
						{"title", "Scheduler"},
						{"description", "The noise schedule that controls how noise is removed during sampling. Karras schedule often produces better results."},
						{"ui:widget", "combo"},
						{"items", scheduler_method_items},
						{"itemCount", scheduler_method_item_count}
					}},
					{"current_type_method", {
						{"type", "integer"},
						{"title", "Quant Type"},
						{"description", "Model precision type. F16 uses less memory, F32 is more accurate. Q4_0/Q8_0 are quantized for even lower memory usage."},
						{"ui:widget", "combo"},
						{"items", type_method_items},
						{"itemCount", type_method_item_count}
					}},
					{"current_prediction_type", {
						{"type", "integer"},
						{"title", "Prediction Type"},
						{"description", "Noise prediction type. EPS_PRED for epsilon prediction (SD 1.5), V_PRED for v-prediction (SDXL), FLUX_FLOW_PRED for FLUX models."},
						{"ui:widget", "combo"},
						{"items", prediction_type_items},
						{"itemCount", prediction_type_item_count}
					}},
					{"seed", {
						{"type", "integer"},
						{"title", "Seed"},
						{"description", "Random seed for reproducible results. Use -1 for random seed, or any positive number for consistent output."},
						{"ui:widget", "input_int"}
					}},
					{"steps", {
						{"type", "integer"},
						{"title", "Steps"},
						{"description", "Number of denoising steps. More steps = higher quality but slower generation. 20-50 is typical range."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 5},
							{"min", 1},
							{"max", 150}
						}}
					}},
					{"eta", {
						{"type", "number"},
						{"title", "ETA"},
						{"description", "Eta parameter for DDIM scheduler. Controls the amount of noise added during sampling. 0.0 = deterministic (DDIM), 1.0 = stochastic (DDPM). Higher values add more randomness."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"denoise", {
						{"type", "number"},
						{"title", "Denoise"},
						{"description", "Denoising strength for img2img. 1.0 = complete denoising (ignores input), 0.0 = no denoising (copies input). 0.6-0.8 is typical."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.01f},
							{"step_fast", 0.1f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"n_threads", {
						{"type", "integer"},
						{"title", "# Threads"},
						{"description", "Number of CPU threads to use. Only affects CPU inference. Set to 0 for auto-detection."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 4},
							{"min", 1},
							{"max", 32}
						}}
					}},
					{"free_params_immediately", {
						{"type", "boolean"},
						{"title", "Free Params"},
						{"description", "Free model parameters immediately after use to save memory. May cause slower performance for consecutive generations."},
						{"ui:widget", "checkbox"}
					}},
					{"offload_params_to_cpu", {
						{"type", "boolean"},
						{"title", "Offload to CPU"},
						{"description", "Force ALL model parameters to CPU when possible. Critical for large models like Wan2.2 - prevents VRAM allocation issues."},
						{"ui:widget", "checkbox"}
					}},
					{"keep_clip_on_cpu", {
						{"type", "boolean"},
						{"title", "CLIP on CPU"},
						{"description", "Keep text encoder on CPU instead of GPU. Saves VRAM but may slow down text processing."},
						{"ui:widget", "checkbox"}
					}},
					{"keep_control_net_on_cpu", {
						{"type", "boolean"},
						{"title", "ControlNet on CPU"},
						{"description", "Keep ControlNet models on CPU. Saves significant VRAM when using ControlNet but reduces performance."},
						{"ui:widget", "checkbox"}
					}},
					{"diffusion_flash_attn", {
						{"type", "boolean"},
						{"title", "Flash Attention"},
						{"description", "Enable Flash Attention optimization for faster and more memory-efficient attention computation. Requires compatible hardware."},
						{"ui:widget", "checkbox"}
					}},
					{"diffusion_conv_direct", {
						{"type", "boolean"},
						{"title", "Direct Diffusion Conv"},
						{"description", "Use direct CPU convolutions for diffusion model. Forces all diffusion operations to CPU."},
						{"ui:widget", "checkbox"}
					}},
					{"vae_conv_direct", {
						{"type", "boolean"},
						{"title", "Direct VAE Conv"},
						{"description", "Use direct CPU convolutions for VAE. Forces all VAE operations to CPU."},
						{"ui:widget", "checkbox"}
					}},
					{"force_sdxl_vae_conv_scale", {
						{"type", "boolean"},
						{"title", "Force SDXL VAE Scale"},
						{"description", "Force SDXL VAE to use scaling in convolutions. May improve quality for SDXL models."},
						{"ui:widget", "checkbox"}
					}},
					{"shifted_timestep", {
						{"type", "integer"},
						{"title", "Shifted Timestep"},
						{"description", "Shifted timestep parameter for video generation or advanced sampling."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 10},
							{"min", 0},
							{"max", 1000}
						}}
					}},
				{"lora_apply_mode", {
					{"type", "integer"},
					{"title", "LoRA Apply Mode"},
					{"description", "When to apply LoRAs (auto, immediate, at runtime)."},
					{"ui:widget", "combo"},
					{"items", lora_apply_mode_items},
					{"itemCount", lora_apply_mode_item_count}
				}},
				{"enable_mmap", {
					{"type", "boolean"},
					{"title", "Enable mmap"},
					{"description", "Use memory-mapped file I/O for model loading. Reduces memory usage but may be slower."},
					{"ui:widget", "checkbox"}
				}},
				{"qwen_image_zero_cond_t", {
					{"type", "boolean"},
					{"title", "Qwen Image Zero Cond T"},
					{"description", "Enable Qwen image zero conditioning for LLM-based image generation."},
					{"ui:widget", "checkbox"}
				}},
				{"max_vram", {
					{"type", "string"},
					{"title", "Max VRAM"},
					{"description", "Maximum VRAM to use (in GiB) or backend assignment spec. '0' = disabled, '-1' = auto."},
					{"ui:widget", "text"}
				}},
				{"stream_layers", {
					{"type", "boolean"},
					{"title", "Stream Layers"},
					{"description", "Enable residency+prefetch streaming when max_vram is set."},
					{"ui:widget", "checkbox"}
				}},
				{"eager_load", {
					{"type", "boolean"},
					{"title", "Eager Load"},
					{"description", "Load all parameters at model-load time instead of lazily."},
					{"ui:widget", "checkbox"}
				}},
				{"backend", {
					{"type", "string"},
					{"title", "Backend"},
					{"description", "Backend to use for computation (e.g., 'cuda', 'cpu')."},
					{"ui:widget", "text"}
				}},
				{"params_backend", {
					{"type", "string"},
					{"title", "Params Backend"},
					{"description", "Backend to store model parameters."},
					{"ui:widget", "text"}
				}},
				{"rpc_servers", {
					{"type", "string"},
					{"title", "RPC Servers"},
					{"description", "RPC server endpoints for distributed inference."},
					{"ui:widget", "text"}
				}},
				{"extra_sample_args", {
					{"type", "string"},
					{"title", "Extra Sample Args"},
					{"description", "Additional arguments for sampling (advanced)."},
					{"ui:widget", "text"}
				}}
			}}
			};
		}

		// Core sampling parameters (from sd_sample_params_t)
		int seed = -1;
		int steps = 20;
		float eta = 0.0f;
		float denoise = 1.0;
		int n_threads = 4;
		bool free_params_immediately = true;

		// Context flags (from sd_ctx_params_t)
		bool offload_params_to_cpu = false;
		bool keep_clip_on_cpu = false;
		bool keep_control_net_on_cpu = false;
		bool diffusion_flash_attn = false;
		bool diffusion_conv_direct = false;
		bool vae_conv_direct = false;
		bool force_sdxl_vae_conv_scale = false;
		int shifted_timestep = 0;

		// Method selections
		sample_method_t current_sample_method = EULER_SAMPLE_METHOD;
		scheduler_t current_scheduler_method = DISCRETE_SCHEDULER;
		sd_type_t current_type_method = SD_TYPE_F16;
		prediction_t current_prediction_type = EPS_PRED;

		// New context parameters
		enum lora_apply_mode_t lora_apply_mode = LORA_APPLY_AUTO;
		bool enable_mmap = true;
		bool qwen_image_zero_cond_t = false;
		std::string max_vram = "-1";
		bool stream_layers = false;
		bool eager_load = false;
		std::string backend;
		std::string params_backend;
		std::string rpc_servers;
		std::vector<float> custom_sigmas;
		std::string extra_sample_args;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["seed"] = &seed;
			properties["steps"] = &steps;
			properties["eta"] = &eta;
			properties["denoise"] = &denoise;
			properties["n_threads"] = &n_threads;
			properties["free_params_immediately"] = &free_params_immediately;
			properties["offload_params_to_cpu"] = &offload_params_to_cpu;
			properties["keep_clip_on_cpu"] = &keep_clip_on_cpu;
			properties["keep_control_net_on_cpu"] = &keep_control_net_on_cpu;
			properties["diffusion_flash_attn"] = &diffusion_flash_attn;
			properties["diffusion_conv_direct"] = &diffusion_conv_direct;
			properties["vae_conv_direct"] = &vae_conv_direct;
			properties["force_sdxl_vae_conv_scale"] = &force_sdxl_vae_conv_scale;
			properties["shifted_timestep"] = &shifted_timestep;
			properties["current_sample_method"] = reinterpret_cast<int*>(&current_sample_method);
			properties["current_scheduler_method"] = reinterpret_cast<int*>(&current_scheduler_method);
			properties["current_type_method"] = reinterpret_cast<int*>(&current_type_method);
			properties["current_prediction_type"] = reinterpret_cast<int*>(&current_prediction_type);
			properties["lora_apply_mode"] = reinterpret_cast<int*>(&lora_apply_mode);
			properties["enable_mmap"] = &enable_mmap;
			properties["qwen_image_zero_cond_t"] = &qwen_image_zero_cond_t;
			properties["max_vram"] = &max_vram;
			properties["stream_layers"] = &stream_layers;
			properties["eager_load"] = &eager_load;
			properties["backend"] = &backend;
			properties["params_backend"] = &params_backend;
			properties["rpc_servers"] = &rpc_servers;
			properties["extra_sample_args"] = &extra_sample_args;
			// custom_sigmas is not directly exposed in UI, but serialized
			return properties;
		}

		SamplerComponent& operator=(const SamplerComponent& other) {
			if (this != &other) {
				seed = other.seed;
				steps = other.steps;
				eta = other.eta;
				denoise = other.denoise;
				n_threads = other.n_threads;
				free_params_immediately = other.free_params_immediately;
				offload_params_to_cpu = other.offload_params_to_cpu;
				keep_clip_on_cpu = other.keep_clip_on_cpu;
				keep_control_net_on_cpu = other.keep_control_net_on_cpu;
				diffusion_flash_attn = other.diffusion_flash_attn;
				diffusion_conv_direct = other.diffusion_conv_direct;
				vae_conv_direct = other.vae_conv_direct;
				force_sdxl_vae_conv_scale = other.force_sdxl_vae_conv_scale;
				shifted_timestep = other.shifted_timestep;
				current_sample_method = other.current_sample_method;
				current_scheduler_method = other.current_scheduler_method;
				current_type_method = other.current_type_method;
				current_prediction_type = other.current_prediction_type;
				lora_apply_mode = other.lora_apply_mode;
				enable_mmap = other.enable_mmap;
				qwen_image_zero_cond_t = other.qwen_image_zero_cond_t;
				max_vram = other.max_vram;
				stream_layers = other.stream_layers;
				eager_load = other.eager_load;
				backend = other.backend;
				params_backend = other.params_backend;
				rpc_servers = other.rpc_servers;
				custom_sigmas = other.custom_sigmas;
				extra_sample_args = other.extra_sample_args;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"seed", seed},
				{"steps", steps},
				{"eta", eta},
				{"denoise", denoise},
				{"n_threads", n_threads},
				{"free_params_immediately", free_params_immediately},
				{"offload_params_to_cpu", offload_params_to_cpu},
				{"keep_clip_on_cpu", keep_clip_on_cpu},
				{"keep_control_net_on_cpu", keep_control_net_on_cpu},
				{"diffusion_flash_attn", diffusion_flash_attn},
				{"diffusion_conv_direct", diffusion_conv_direct},
				{"vae_conv_direct", vae_conv_direct},
				{"force_sdxl_vae_conv_scale", force_sdxl_vae_conv_scale},
				{"shifted_timestep", shifted_timestep},
				{"current_sample_method", static_cast<int>(current_sample_method)},
				{"current_scheduler_method", static_cast<int>(current_scheduler_method)},
				{"current_type_method", static_cast<int>(current_type_method)},
				{"current_prediction_type", static_cast<int>(current_prediction_type)},
				{"lora_apply_mode", static_cast<int>(lora_apply_mode)},
				{"enable_mmap", enable_mmap},
				{"qwen_image_zero_cond_t", qwen_image_zero_cond_t},
				{"max_vram", max_vram},
				{"stream_layers", stream_layers},
				{"eager_load", eager_load},
				{"backend", backend},
				{"params_backend", params_backend},
				{"rpc_servers", rpc_servers},
				{"custom_sigmas", custom_sigmas},
				{"extra_sample_args", extra_sample_args}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				for (auto it = j.begin(); it != j.end(); ++it) {
					if (it.key() == compName) {
						componentData = it.value();
						break;
					}
				}
				if (componentData.empty()) {
					componentData = j;
				}
			}

			if (componentData.contains("seed"))
				seed = componentData["seed"];
			if (componentData.contains("steps"))
				steps = componentData["steps"];
			if (componentData.contains("eta"))
				eta = componentData["eta"];
			if (componentData.contains("denoise"))
				denoise = componentData["denoise"];
			if (componentData.contains("n_threads"))
				n_threads = componentData["n_threads"];
			if (componentData.contains("free_params_immediately"))
				free_params_immediately = componentData["free_params_immediately"].get<bool>();
			if (componentData.contains("offload_params_to_cpu"))
				offload_params_to_cpu = componentData["offload_params_to_cpu"].get<bool>();
			if (componentData.contains("keep_clip_on_cpu"))
				keep_clip_on_cpu = componentData["keep_clip_on_cpu"].get<bool>();
			if (componentData.contains("keep_control_net_on_cpu"))
				keep_control_net_on_cpu = componentData["keep_control_net_on_cpu"].get<bool>();
			if (componentData.contains("diffusion_flash_attn"))
				diffusion_flash_attn = componentData["diffusion_flash_attn"].get<bool>();
			if (componentData.contains("diffusion_conv_direct"))
				diffusion_conv_direct = componentData["diffusion_conv_direct"].get<bool>();
			if (componentData.contains("vae_conv_direct"))
				vae_conv_direct = componentData["vae_conv_direct"].get<bool>();
			if (componentData.contains("force_sdxl_vae_conv_scale"))
				force_sdxl_vae_conv_scale = componentData["force_sdxl_vae_conv_scale"].get<bool>();
			if (componentData.contains("shifted_timestep"))
				shifted_timestep = componentData["shifted_timestep"];
			if (componentData.contains("current_sample_method"))
				current_sample_method = static_cast<sample_method_t>(componentData["current_sample_method"].get<int>());
			if (componentData.contains("current_scheduler_method"))
				current_scheduler_method = static_cast<scheduler_t>(componentData["current_scheduler_method"].get<int>());
			if (componentData.contains("current_type_method"))
				current_type_method = static_cast<sd_type_t>(componentData["current_type_method"].get<int>());
			if (componentData.contains("current_prediction_type"))
				current_prediction_type = static_cast<prediction_t>(componentData["current_prediction_type"].get<int>());
			if (componentData.contains("lora_apply_mode"))
				lora_apply_mode = static_cast<lora_apply_mode_t>(componentData["lora_apply_mode"].get<int>());
			if (componentData.contains("enable_mmap"))
				enable_mmap = componentData["enable_mmap"].get<bool>();
			if (componentData.contains("qwen_image_zero_cond_t"))
				qwen_image_zero_cond_t = componentData["qwen_image_zero_cond_t"].get<bool>();
			if (componentData.contains("max_vram"))
				max_vram = componentData["max_vram"].get<std::string>();
			if (componentData.contains("stream_layers"))
				stream_layers = componentData["stream_layers"].get<bool>();
			if (componentData.contains("eager_load"))
				eager_load = componentData["eager_load"].get<bool>();
			if (componentData.contains("backend"))
				backend = componentData["backend"].get<std::string>();
			if (componentData.contains("params_backend"))
				params_backend = componentData["params_backend"].get<std::string>();
			if (componentData.contains("rpc_servers"))
				rpc_servers = componentData["rpc_servers"].get<std::string>();
			if (componentData.contains("custom_sigmas") && componentData["custom_sigmas"].is_array()) {
				custom_sigmas = componentData["custom_sigmas"].get<std::vector<float>>();
			}
			if (componentData.contains("extra_sample_args"))
				extra_sample_args = componentData["extra_sample_args"].get<std::string>();
		}
	};

} // namespace ECS