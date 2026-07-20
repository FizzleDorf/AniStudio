#pragma once

#include "BaseComponent.hpp"
#include "ModelComponents.hpp"
#include "stable-diffusion.h"
#include "DiffusionOptions.hpp"
#include <string>

namespace ECS {

	struct VideoParamsComponent : public BaseComponent {
		VideoParamsComponent() {
			compName = "VideoParams";

			schema = {
				{"title", "Video Generation Settings"},
				{"type", "object"},
				{"propertyOrder", {"video_frames", "vace_strength", "motion_bucket_id", "fps", "augmentation_level",
								  "min_cfg", "moe_boundary", "flow_shift"}},
				{"properties", {
					{"video_frames", {
						{"type", "integer"},
						{"title", "Video Frames"},
						{"description", "Number of frames to generate. More frames = longer video but slower generation."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 8},
							{"min", 1},
							{"max", 120}
						}}
					}},
					{"vace_strength", {
						{"type", "number"},
						{"title", "VACE Strength"},
						{"description", "Video Auto-Conditioning Enhancement strength. Controls temporal consistency between frames. 0.0 = disabled, higher values = stronger consistency."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"motion_bucket_id", {
						{"type", "integer"},
						{"title", "Motion Bucket ID"},
						{"description", "Controls motion intensity. Higher values = more motion. Typical range: 100-200."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 10},
							{"min", 1},
							{"max", 255}
						}}
					}},
					{"fps", {
						{"type", "integer"},
						{"title", "FPS"},
						{"description", "Frames per second for the output video. Standard values: 6, 12, 24, 30."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 6},
							{"min", 1},
							{"max", 60}
						}}
					}},
					{"augmentation_level", {
						{"type", "number"},
						{"title", "Augmentation Level"},
						{"description", "Data augmentation strength for video generation. Higher values add more variation."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 10.0f}
						}}
					}},
					{"min_cfg", {
						{"type", "number"},
						{"title", "Min CFG"},
						{"description", "Minimum CFG scale for dynamic CFG scheduling. Used in some Wan models."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 20.0f}
						}}
					}},
					{"moe_boundary", {
						{"type", "number"},
						{"title", "MoE Boundary"},
						{"description", "Mixture of Experts boundary for Wan 2.2 models. Controls which expert is used."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"flow_shift", {
						{"type", "number"},
						{"title", "Flow Shift"},
						{"description", "Flow matching shift parameter for Wan models. Affects generation dynamics."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 1.0f},
							{"format", "%.1f"},
							{"min", 0.0f},
							{"max", 10.0f}
						}}
					}}
				}}
			};
		}

		// Video generation parameters
		int video_frames = 25;
		float vace_strength = 0.0f;
		int motion_bucket_id = 127;
		int fps = 6;
		float augmentation_level = 0.0f;
		float min_cfg = 1.0f;
		float moe_boundary = 0.0f;
		float flow_shift = 3.0f;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"video_frames", &video_frames},
				{"vace_strength", &vace_strength},
				{"motion_bucket_id", &motion_bucket_id},
				{"fps", &fps},
				{"augmentation_level", &augmentation_level},
				{"min_cfg", &min_cfg},
				{"moe_boundary", &moe_boundary},
				{"flow_shift", &flow_shift}
			};
		}

		VideoParamsComponent& operator=(const VideoParamsComponent& other) {
			if (this != &other) {
				video_frames = other.video_frames;
				vace_strength = other.vace_strength;
				motion_bucket_id = other.motion_bucket_id;
				fps = other.fps;
				augmentation_level = other.augmentation_level;
				min_cfg = other.min_cfg;
				moe_boundary = other.moe_boundary;
				flow_shift = other.flow_shift;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"video_frames", video_frames},
				{"vace_strength", vace_strength},
				{"motion_bucket_id", motion_bucket_id},
				{"fps", fps},
				{"augmentation_level", augmentation_level},
				{"min_cfg", min_cfg},
				{"moe_boundary", moe_boundary},
				{"flow_shift", flow_shift}
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

			if (componentData.contains("video_frames"))
				video_frames = componentData["video_frames"];
			if (componentData.contains("vace_strength"))
				vace_strength = componentData["vace_strength"];
			if (componentData.contains("motion_bucket_id"))
				motion_bucket_id = componentData["motion_bucket_id"];
			if (componentData.contains("fps"))
				fps = componentData["fps"];
			if (componentData.contains("augmentation_level"))
				augmentation_level = componentData["augmentation_level"];
			if (componentData.contains("min_cfg"))
				min_cfg = componentData["min_cfg"];
			if (componentData.contains("moe_boundary"))
				moe_boundary = componentData["moe_boundary"];
			if (componentData.contains("flow_shift"))
				flow_shift = componentData["flow_shift"];
		}
	};

	// High Noise Sampler Component for Wan 2.2 dual sampling
	struct HighNoiseSamplerComponent : public BaseComponent {
		HighNoiseSamplerComponent() {
			compName = "HighNoiseSampler";

			schema = {
				{"title", "High Noise Sampler Settings"},
				{"type", "object"},
				{"propertyOrder", {"high_noise_sample_method", "high_noise_scheduler_method",
								  "high_noise_cfg", "high_noise_steps", "high_noise_eta"}},
				{"properties", {
					{"high_noise_sample_method", {
						{"type", "integer"},
						{"title", "High Noise Sampler"},
						{"description", "Sampling method for high noise model in Wan 2.2 dual-model setup."},
						{"ui:widget", "combo"},
						{"items", sample_method_items},
						{"itemCount", sample_method_item_count}
					}},
					{"high_noise_scheduler_method", {
						{"type", "integer"},
						{"title", "High Noise Scheduler"},
						{"description", "Scheduler for high noise model sampling."},
						{"ui:widget", "combo"},
						{"items", scheduler_method_items},
						{"itemCount", scheduler_method_item_count}
					}},
					{"high_noise_cfg", {
						{"type", "number"},
						{"title", "High Noise CFG"},
						{"description", "CFG scale for high noise model. Usually same as main CFG."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.5f},
							{"step_fast", 1.0f},
							{"format", "%.2f"},
							{"min", 1.0f},
							{"max", 20.0f}
						}}
					}},
					{"high_noise_steps", {
						{"type", "integer"},
						{"title", "High Noise Steps"},
						{"description", "Number of steps for high noise model. Usually fewer than main steps."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 2},
							{"min", 1},
							{"max", 50}
						}}
					}},
					{"high_noise_eta", {
						{"type", "number"},
						{"title", "High Noise ETA"},
						{"description", "Eta parameter for high noise sampler. Controls noise addition during sampling."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}}
				}}
			};
		}

		// High noise sampling parameters
		sample_method_t high_noise_sample_method = EULER_SAMPLE_METHOD;
		scheduler_t high_noise_scheduler_method = DISCRETE_SCHEDULER;
		int high_noise_steps = 8;
		float high_noise_cfg = 3.5f;
		float high_noise_eta = 0.0f;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"high_noise_sample_method", reinterpret_cast<int*>(&high_noise_sample_method)},
				{"high_noise_scheduler_method", reinterpret_cast<int*>(&high_noise_scheduler_method)},
				{"high_noise_cfg", &high_noise_cfg},
				{"high_noise_steps", &high_noise_steps},
				{"high_noise_eta", &high_noise_eta}
			};
		}

		HighNoiseSamplerComponent& operator=(const HighNoiseSamplerComponent& other) {
			if (this != &other) {
				high_noise_sample_method = other.high_noise_sample_method;
				high_noise_scheduler_method = other.high_noise_scheduler_method;
				high_noise_cfg = other.high_noise_cfg;
				high_noise_steps = other.high_noise_steps;
				high_noise_eta = other.high_noise_eta;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"high_noise_sample_method", static_cast<int>(high_noise_sample_method)},
				{"high_noise_scheduler_method", static_cast<int>(high_noise_scheduler_method)},
				{"high_noise_cfg", high_noise_cfg},
				{"high_noise_steps", high_noise_steps},
				{"high_noise_eta", high_noise_eta}
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

			if (componentData.contains("high_noise_sample_method"))
				high_noise_sample_method = static_cast<sample_method_t>(componentData["high_noise_sample_method"].get<int>());
			if (componentData.contains("high_noise_scheduler_method"))
				high_noise_scheduler_method = static_cast<scheduler_t>(componentData["high_noise_scheduler_method"].get<int>());
			if (componentData.contains("high_noise_cfg"))
				high_noise_cfg = componentData["high_noise_cfg"];
			if (componentData.contains("high_noise_steps"))
				high_noise_steps = componentData["high_noise_steps"];
			if (componentData.contains("high_noise_eta"))
				high_noise_eta = componentData["high_noise_eta"];
		}
	};

} // namespace ECS