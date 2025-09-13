#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "Constants.hpp"
#include <string>

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
					"seed", "cfg", "steps", "denoise", "n_threads", "free_params_immediately",
					"offload_params_to_cpu", "keep_clip_on_cpu", "diffusion_flash_attn", "diffusion_conv_direct", "vae_conv_direct"
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
					{"seed", {
						{"type", "integer"},
						{"title", "Seed"},
						{"description", "Random seed for reproducible results. Use -1 for random seed, or any positive number for consistent output."},
						{"ui:widget", "input_int"}
					}},
					{"cfg", {
						{"type", "number"},
						{"title", "CFG"},
						{"description", "Classifier-Free Guidance scale. Higher values follow the prompt more closely but may reduce image quality. Typical range: 1-20."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.5f},
							{"step_fast", 1.0f},
							{"format", "%.2f"}
						}}
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
					}}
				}}
			};
		}

		// Core sampling parameters
		int seed = -1;
		int steps = 20;
		float denoise = 1.0;
		float cfg = 7.0;
		int n_threads = 4;
		bool free_params_immediately = true;

		// System-wide control flags
		bool offload_params_to_cpu = false;
		bool keep_clip_on_cpu = true;
		bool diffusion_flash_attn = false;
		bool diffusion_conv_direct = false;
		bool vae_conv_direct = false;

		// Method selections
		sample_method_t current_sample_method = EULER;
		scheduler_t current_scheduler_method = DEFAULT;
		sd_type_t current_type_method = SD_TYPE_F16;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["seed"] = &seed;
			properties["steps"] = &steps;
			properties["denoise"] = &denoise;
			properties["cfg"] = &cfg;
			properties["n_threads"] = &n_threads;
			properties["free_params_immediately"] = &free_params_immediately;
			properties["offload_params_to_cpu"] = &offload_params_to_cpu;
			properties["keep_clip_on_cpu"] = &keep_clip_on_cpu;
			properties["diffusion_flash_attn"] = &diffusion_flash_attn;
			properties["diffusion_conv_direct"] = &diffusion_conv_direct;
			properties["vae_conv_direct"] = &vae_conv_direct;

			properties["current_sample_method"] = reinterpret_cast<int*>(&current_sample_method);
			properties["current_scheduler_method"] = reinterpret_cast<int*>(&current_scheduler_method);
			properties["current_type_method"] = reinterpret_cast<int*>(&current_type_method);

			return properties;
		}

		SamplerComponent& operator=(const SamplerComponent& other) {
			if (this != &other) {
				seed = other.seed;
				steps = other.steps;
				denoise = other.denoise;
				cfg = other.cfg;
				n_threads = other.n_threads;
				free_params_immediately = other.free_params_immediately;
				offload_params_to_cpu = other.offload_params_to_cpu;
				keep_clip_on_cpu = other.keep_clip_on_cpu;
				diffusion_flash_attn = other.diffusion_flash_attn;
				diffusion_conv_direct = other.diffusion_conv_direct;
				vae_conv_direct = other.vae_conv_direct;
				current_sample_method = other.current_sample_method;
				current_scheduler_method = other.current_scheduler_method;
				current_type_method = other.current_type_method;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"seed", seed},
				{"steps", steps},
				{"cfg", cfg},
				{"denoise", denoise},
				{"n_threads", n_threads},
				{"free_params_immediately", free_params_immediately},
				{"offload_params_to_cpu", offload_params_to_cpu},
				{"keep_clip_on_cpu", keep_clip_on_cpu},
				{"diffusion_flash_attn", diffusion_flash_attn},
				{"diffusion_conv_direct", diffusion_conv_direct},
				{"vae_conv_direct", vae_conv_direct},
				{"current_sample_method", static_cast<int>(current_sample_method)},
				{"current_scheduler_method", static_cast<int>(current_scheduler_method)},
				{"current_type_method", static_cast<int>(current_type_method)}
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
			if (componentData.contains("cfg"))
				cfg = componentData["cfg"];
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
			if (componentData.contains("diffusion_flash_attn"))
				diffusion_flash_attn = componentData["diffusion_flash_attn"].get<bool>();
			if (componentData.contains("diffusion_conv_direct"))
				diffusion_conv_direct = componentData["diffusion_conv_direct"].get<bool>();
			if (componentData.contains("vae_conv_direct"))
				vae_conv_direct = componentData["vae_conv_direct"].get<bool>();
			if (componentData.contains("current_sample_method"))
				current_sample_method = static_cast<sample_method_t>(componentData["current_sample_method"]);
			if (componentData.contains("current_scheduler_method"))
				current_scheduler_method = static_cast<scheduler_t>(componentData["current_scheduler_method"]);
			if (componentData.contains("current_type_method"))
				current_type_method = static_cast<sd_type_t>(componentData["current_type_method"]);
		}
	};

} // namespace ECS