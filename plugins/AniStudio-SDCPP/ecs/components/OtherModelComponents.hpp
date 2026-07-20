#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	// Photo Maker Component (for identity embedding/face swapping)
	struct PhotoMakerComponent : public BaseModelComponent {
		PhotoMakerComponent() {
			compName = "PhotoMaker";

			schema = {
				{"title", "Photo Maker"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Photo Maker"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "Photo Maker Models"},
							{"dialogDefaultPath", "PhotoMaker"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for Photo Maker identity embedding/face swapping models"}
						}}
					}}
				}}
			};
		}

		PhotoMakerComponent& operator=(const PhotoMakerComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// ControlNet Component
	struct ControlNetComponent : public ECS::BaseModelComponent {
		ControlNetComponent() {
			compName = "ControlNet";

			schema = {
				{"title", "ControlNet Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "cnStrength", "applyStart", "applyEnd", "keep_control_net_cpu"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "ControlNet"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "ControlNet Models"},
							{"dialogDefaultPath", "ControlNet"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for ControlNet model files"}
						}}
					}},
					{"cnStrength", {
						{"type", "number"},
						{"title", "Strength"},
						{"description", "ControlNet influence strength. Higher values make the model follow the control input more strictly."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.2f},
							{"format", "%.1f"},
							{"min", 0.0f},
							{"max", 2.0f}
						}}
					}},
					{"applyStart", {
						{"type", "number"},
						{"title", "Start Step"},
						{"description", "When to start applying ControlNet (0.0 = from beginning). Useful for letting initial generation be more free."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.2f},
							{"format", "%.1f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"applyEnd", {
						{"type", "number"},
						{"title", "End Step"},
						{"description", "When to stop applying ControlNet (1.0 = until end). Useful for letting final details be more natural."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.2f},
							{"format", "%.1f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"keep_control_net_cpu", {
						{"type", "boolean"},
						{"title", "Keep ControlNet on CPU"},
						{"description", "Keep ControlNet models on CPU. Saves significant VRAM when using ControlNet but reduces performance."},
						{"ui:widget", "checkbox"}
					}}
				}}
			};
		}

		float cnStrength = 1.0f;
		float applyStart = 0.0f;
		float applyEnd = 1.0f;
		bool keep_control_net_cpu = false;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"cnStrength", &cnStrength},
				{"applyStart", &applyStart},
				{"applyEnd", &applyEnd},
				{"keep_control_net_cpu", &keep_control_net_cpu}
			};
		}

		ControlNetComponent& operator=(const ControlNetComponent& other) {
			if (this != &other) {
				modelName = other.modelName;
				modelPath = other.modelPath;
				isModelLoaded = other.isModelLoaded;
				cnStrength = other.cnStrength;
				applyStart = other.applyStart;
				applyEnd = other.applyEnd;
				keep_control_net_cpu = other.keep_control_net_cpu;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"cnStrength", cnStrength},
				{"applyStart", applyStart},
				{"applyEnd", applyEnd},
				{"keep_control_net_cpu", keep_control_net_cpu}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseModelComponent::Deserialize(j);

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

			if (componentData.contains("cnStrength"))
				cnStrength = componentData["cnStrength"];
			if (componentData.contains("applyStart"))
				applyStart = componentData["applyStart"];
			if (componentData.contains("applyEnd"))
				applyEnd = componentData["applyEnd"];
			if (componentData.contains("keep_control_net_cpu"))
				keep_control_net_cpu = componentData["keep_control_net_cpu"].get<bool>();
		}
	};

	// Upscale Model Component
	struct EsrganComponent : public BaseModelComponent {
		EsrganComponent() {
			compName = "Esrgan";

			schema = {
				{"title", "ESRGAN Upscaler"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "upscaleFactor", "preserveAspectRatio"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Upscale Model"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".pth,.safetensors,.pt"},
							{"filterName", "Upscale Models"},
							{"dialogDefaultPath", "Upscale"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for ESRGAN upscale model files (.pth, .safetensors, .pt)"}
						}}
					}},
					{"upscaleFactor", {
						{"type", "integer"},
						{"title", "Upscale Factor"},
						{"description", "How much to upscale the image. 2x = double size, 4x = quadruple size. Higher factors take more time and memory."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 2},
							{"min", 1},
							{"max", 8}
						}}
					}},
					{"tileSize", {
						{"type", "integer"},
						{"title", "Upscale Factor"},
						{"description", "How the upscale model tiles the image to upscale. larger sizes upscale faster but use more memory"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 8},
							{"step_fast", 16},
							{"min", 64},
							{"max", 1024}
						}}
					}}
				}}
			};
		}

		uint32_t upscaleFactor = 2;
		uint32_t tileSize = 512;
		bool preserveAspectRatio = true;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"upscaleFactor", reinterpret_cast<int*>(&upscaleFactor)},
				{"preserveAspectRatio", &preserveAspectRatio}
			};
		}

		EsrganComponent& operator=(const EsrganComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
				upscaleFactor = other.upscaleFactor;
				preserveAspectRatio = other.preserveAspectRatio;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"upscaleFactor", upscaleFactor},
				{"preserveAspectRatio", preserveAspectRatio}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseModelComponent::Deserialize(j);

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

			if (componentData.contains("upscaleFactor"))
				upscaleFactor = componentData["upscaleFactor"];
			if (componentData.contains("preserveAspectRatio"))
				preserveAspectRatio = componentData["preserveAspectRatio"].get<bool>();
		}
	};

	// LoRa Component
	struct LoraComponent : public ECS::BaseModelComponent {
		LoraComponent() {
			compName = "Lora";
			modelPath = "";
			schema = {
				{"title", "LoRA Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "lora_apply_mode"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "LoRA Directory"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "directory"},
							{"dialogDefaultPath", "Lora"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for root LoRA directory"}
						}}
					}},
					{"lora_apply_mode", {
						{"type", "integer"},
						{"title", "LoRA Apply Mode"},
						{"description", "When to apply LoRAs (auto, immediately, at runtime)."},
						{"ui:widget", "combo"},
						{"items", lora_apply_mode_items},
						{"itemCount", lora_apply_mode_item_count}
					}}
				}}
			};
		}

		enum lora_apply_mode_t lora_apply_mode = LORA_APPLY_AUTO;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"lora_apply_mode", reinterpret_cast<int*>(&lora_apply_mode)}
			};
		}

		LoraComponent& operator=(const LoraComponent& other) {
			if (this != &other) {
				modelName = other.modelName;
				modelPath = other.modelPath;
				isModelLoaded = other.isModelLoaded;
				lora_apply_mode = other.lora_apply_mode;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"lora_apply_mode", static_cast<int>(lora_apply_mode)}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseModelComponent::Deserialize(j);
			if (j.contains(compName)) {
				auto comp = j[compName];
				if (comp.contains("lora_apply_mode"))
					lora_apply_mode = static_cast<lora_apply_mode_t>(comp["lora_apply_mode"].get<int>());
			}
		}
	};
}