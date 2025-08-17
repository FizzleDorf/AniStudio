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

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include "Constants.hpp"
#include <string>

namespace ECS {

	struct SamplerComponent : public ECS::BaseComponent {
		SamplerComponent() {
			compName = "Sampler";

			// Enhanced schema WITHOUT table layout - just direct widget rendering
			schema = {
		{"title", "Sampler Settings"},
		{"type", "object"},
		{"propertyOrder", {
			"current_sample_method", "current_scheduler_method", "seed",
			"cfg", "steps", "denoise", "n_threads", "free_params_immediately",
			"keep_clip_on_cpu", "keep_control_net_cpu", "keep_vae_on_cpu", "diffusion_flash_attn"
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
			{"current_rng_type", {
				{"type", "integer"},
				{"title", "RNG Type"},
				{"description", "Random number generator type. CUDA RNG provides different results than CPU RNG for the same seed."},
				{"ui:widget", "combo"},
				{"items", type_rng_items},
				{"itemCount", type_rng_item_count}
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
			{"keep_clip_on_cpu", {
				{"type", "boolean"},
				{"title", "CLIP on CPU"},
				{"description", "Keep text encoder on CPU instead of GPU. Saves VRAM but may slow down text processing."},
				{"ui:widget", "checkbox"}
			}},
			{"keep_control_net_cpu", {
				{"type", "boolean"},
				{"title", "ControlNet on CPU"},
				{"description", "Keep ControlNet models on CPU. Saves significant VRAM when using ControlNet but reduces performance."},
				{"ui:widget", "checkbox"}
			}},
			{"keep_vae_on_cpu", {
				{"type", "boolean"},
				{"title", "VAE on CPU"},
				{"description", "Keep VAE on CPU instead of GPU. Can help with memory issues but significantly slows down encoding/decoding."},
				{"ui:widget", "checkbox"}
			}},
			{"diffusion_flash_attn", {
				{"type", "boolean"},
				{"title", "Flash Attention"},
				{"description", "Enable Flash Attention optimization for faster and more memory-efficient attention computation. Requires compatible hardware."},
				{"ui:widget", "checkbox"}
			}}
		}}
			};
		}

		// Core sampling parameters
		int steps = 20;
		float denoise = 1.0;
		float cfg = 7.0;
		int seed = -1;
		int n_threads = 4;
		bool free_params_immediately = true;

		// NEW: Backend control parameters for updated API
		bool keep_clip_on_cpu = true;
		bool keep_control_net_cpu = false;
		bool keep_vae_on_cpu = false;
		bool diffusion_flash_attn = false;

		// Method selections
		sample_method_t current_sample_method = sample_method_t::EULER;
		schedule_t current_scheduler_method = schedule_t::DEFAULT;
		sd_type_t current_type_method = sd_type_t::SD_TYPE_F16;
		rng_type_t current_rng_type = rng_type_t::STD_DEFAULT_RNG;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["steps"] = &steps;
			properties["denoise"] = &denoise;
			properties["cfg"] = &cfg;
			properties["seed"] = &seed;
			properties["n_threads"] = &n_threads;
			properties["free_params_immediately"] = &free_params_immediately;

			// NEW: Backend control parameters
			properties["keep_clip_on_cpu"] = &keep_clip_on_cpu;
			properties["keep_control_net_cpu"] = &keep_control_net_cpu;
			properties["keep_vae_on_cpu"] = &keep_vae_on_cpu;
			properties["diffusion_flash_attn"] = &diffusion_flash_attn;

			// Need to use reinterpret_cast for the enum types
			properties["current_sample_method"] = reinterpret_cast<int*>(&current_sample_method);
			properties["current_scheduler_method"] = reinterpret_cast<int*>(&current_scheduler_method);
			properties["current_type_method"] = reinterpret_cast<int*>(&current_type_method);
			properties["current_rng_type"] = reinterpret_cast<int*>(&current_rng_type);

			return properties;
		}

		SamplerComponent& operator=(const SamplerComponent& other) {
			if (this != &other) {
				steps = other.steps;
				denoise = other.denoise;
				cfg = other.cfg;
				seed = other.seed;
				n_threads = other.n_threads;
				free_params_immediately = other.free_params_immediately;

				// NEW: Copy backend control parameters
				keep_clip_on_cpu = other.keep_clip_on_cpu;
				keep_control_net_cpu = other.keep_control_net_cpu;
				keep_vae_on_cpu = other.keep_vae_on_cpu;
				diffusion_flash_attn = other.diffusion_flash_attn;

				current_sample_method = other.current_sample_method;
				current_scheduler_method = other.current_scheduler_method;
				current_type_method = other.current_type_method;
				current_rng_type = other.current_rng_type;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName,
					 {{"seed", seed},
					  {"steps", steps},
					  {"cfg", cfg},
					  {"denoise", denoise},
					  {"n_threads", n_threads},
					  {"free_params_immediately", free_params_immediately},
					  {"keep_clip_on_cpu", keep_clip_on_cpu},
					  {"keep_control_net_cpu", keep_control_net_cpu},
					  {"keep_vae_on_cpu", keep_vae_on_cpu},
					  {"diffusion_flash_attn", diffusion_flash_attn},
					  {"current_sample_method", static_cast<int>(current_sample_method)},
					  {"current_scheduler_method", static_cast<int>(current_scheduler_method)},
					  {"current_type_method", static_cast<int>(current_type_method)},
					  {"current_rng_type", static_cast<int>(current_rng_type)}}} };
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
			if (componentData.contains("keep_clip_on_cpu"))
				keep_clip_on_cpu = componentData["keep_clip_on_cpu"].get<bool>();
			if (componentData.contains("keep_control_net_cpu"))
				keep_control_net_cpu = componentData["keep_control_net_cpu"].get<bool>();
			if (componentData.contains("keep_vae_on_cpu"))
				keep_vae_on_cpu = componentData["keep_vae_on_cpu"].get<bool>();
			if (componentData.contains("diffusion_flash_attn"))
				diffusion_flash_attn = componentData["diffusion_flash_attn"].get<bool>();

			if (componentData.contains("current_sample_method"))
				current_sample_method = static_cast<sample_method_t>(componentData["current_sample_method"]);
			if (componentData.contains("current_scheduler_method"))
				current_scheduler_method = static_cast<schedule_t>(componentData["current_scheduler_method"]);
			if (componentData.contains("current_type_method"))
				current_type_method = static_cast<sd_type_t>(componentData["current_type_method"]);
			if (componentData.contains("current_rng_type"))
				current_rng_type = static_cast<rng_type_t>(componentData["current_rng_type"]);
		}
	};

} // namespace ECS