#pragma once

#include "BaseModelComponent.hpp"
#include "FilePaths.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {
	// VAE Component
	struct VaeComponent : public BaseModelComponent {
		VaeComponent() {
			compName = "Vae";

			schema = {
				{"title", "VAE Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "isTiled", "tile_size_x", "tile_size_y", "target_overlap", "rel_size_x", "rel_size_y", "keep_vae_on_cpu", "vae_decode_only"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "VAE"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "VAE Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Vae")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for VAE model files (.safetensors, .ckpt, .pt)"}
						}}
					}},
					{"isTiled", {
						{"type", "boolean"},
						{"title", "Tiled VAE"},
						{"description", "Enable tiled VAE processing to reduce memory usage for large images. Trades speed for memory efficiency."},
						{"ui:widget", "checkbox"}
					}},
					{"tile_size_x", {
						{"type", "integer"},
						{"title", "Tile Width"},
						{"description", "Width of each tile in pixels. Smaller values use less memory but are slower."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 8},
							{"step_fast", 16},
							{"min", 32},
							{"max", 512}
						}}
					}},
					{"tile_size_y", {
						{"type", "integer"},
						{"title", "Tile Height"},
						{"description", "Height of each tile in pixels. Smaller values use less memory but are slower."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 8},
							{"step_fast", 16},
							{"min", 32},
							{"max", 512}
						}}
					}},
					{"target_overlap", {
						{"type", "number"},
						{"title", "Tile Overlap"},
						{"description", "Overlap between tiles as a fraction (0.0-1.0). Higher values reduce seams but use more memory."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"min", 0.0f},
							{"max", 0.5f}
						}}
					}},
					{"rel_size_x", {
						{"type", "number"},
						{"title", "Relative Width"},
						{"description", "Relative width scaling factor for tiles."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 8.0f},
							{"step_fast", 16.0f},
							{"min", 8.0f},
							{"max", 512.0f}
						}}
					}},
					{"rel_size_y", {
						{"type", "number"},
						{"title", "Relative Height"},
						{"description", "Relative height scaling factor for tiles."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 8.0f},
							{"step_fast", 16.0f},
							{"min", 8.0f},
							{"max", 512.0f}
						}}
					}},
					{"keep_vae_on_cpu", {
						{"type", "boolean"},
						{"title", "Keep VAE on CPU"},
						{"description", "Keep VAE on CPU instead of GPU to save VRAM but significantly reduce performance."},
						{"ui:widget", "checkbox"}
					}},
					{"vae_decode_only", {
						{"type", "boolean"},
						{"title", "VAE Decode Only"},
						{"description", "Only use VAE for decoding (not encoding). Useful for txt2img workflows to save memory."},
						{"ui:widget", "checkbox"}
					}}
				}}
			};
		}

		bool isTiled = false;
		int tile_size_x = 64;
		int tile_size_y = 64;
		float target_overlap = 0;
		float rel_size_x = 64;
		float rel_size_y = 64;
		bool keep_vae_on_cpu = false;
		bool vae_decode_only = false;

		void RefreshSchema() override {
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Vae");
				}
			}
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"isTiled", &isTiled},
				{"tile_size_x", &tile_size_x},
				{"tile_size_y", &tile_size_y},
				{"target_overlap", &target_overlap},
				{"rel_size_x", &rel_size_x},
				{"rel_size_y", &rel_size_y},
				{"keep_vae_on_cpu", &keep_vae_on_cpu},
				{"vae_decode_only", &vae_decode_only}
			};
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Vae");
		}

		VaeComponent& operator=(const VaeComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
				isTiled = other.isTiled;
				tile_size_x = other.tile_size_x;
				tile_size_y = other.tile_size_y;
				target_overlap = other.target_overlap;
				rel_size_x = other.rel_size_x;
				rel_size_y = other.rel_size_y;
				keep_vae_on_cpu = other.keep_vae_on_cpu;
				vae_decode_only = other.vae_decode_only;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"isTiled", isTiled},
				{"tile_size_x", tile_size_x},
				{"tile_size_y", tile_size_y},
				{"target_overlap", target_overlap},
				{"rel_size_x", rel_size_x},
				{"rel_size_y", rel_size_y},
				{"keep_vae_on_cpu", keep_vae_on_cpu},
				{"vae_decode_only", vae_decode_only}
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

			if (componentData.contains("isTiled"))
				isTiled = componentData["isTiled"].get<bool>();
			if (componentData.contains("tile_size_x"))
				tile_size_x = componentData["tile_size_x"].get<int>();
			if (componentData.contains("tile_size_y"))
				tile_size_y = componentData["tile_size_y"].get<int>();
			if (componentData.contains("target_overlap"))
				target_overlap = componentData["target_overlap"].get<float>();
			if (componentData.contains("rel_size_x"))
				rel_size_x = componentData["rel_size_x"].get<float>();
			if (componentData.contains("rel_size_y"))
				rel_size_y = componentData["rel_size_y"].get<float>();
			if (componentData.contains("keep_vae_on_cpu"))
				keep_vae_on_cpu = componentData["keep_vae_on_cpu"].get<bool>();
			if (componentData.contains("vae_decode_only"))
				vae_decode_only = componentData["vae_decode_only"].get<bool>();
		}
	};

	// Fast Vae Model loader
	struct TaesdComponent : public BaseModelComponent {
		TaesdComponent() {
			compName = "Taesd";

			schema = {
				{"title", "TAESD Fast VAE"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "TAESD"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "TAESD Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Vae")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for TAESD fast VAE files"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Vae");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Vae");
		}

		TaesdComponent& operator=(const TaesdComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};
}