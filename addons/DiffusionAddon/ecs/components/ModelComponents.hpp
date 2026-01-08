#pragma once

#include "BaseModelComponent.hpp"
#include "FilePathService.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	// Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
	struct CheckpointComponent : public BaseModelComponent {
		CheckpointComponent() {
			compName = "Checkpoint";
			schema = {
				{"title", "Checkpoint Model"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Checkpoint"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "Checkpoint Models"},
							{"dialogDefaultPath", "Checkpoint"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for checkpoint model files (.safetensors, .ckpt, .pt, .gguf)"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Checkpoint path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePathService::GetPath("Checkpoint");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePathService::GetPath("Checkpoint");
		}

		CheckpointComponent& operator=(const CheckpointComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Diffusion Model only (Flux)
	struct DiffusionModelComponent : public BaseModelComponent {
		DiffusionModelComponent() {
			compName = "DiffusionModel";
			schema = {
				{"title", "UNet/Diffusion Model"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "UNet"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "UNet Models"},
							{"dialogDefaultPath", "Unet"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for UNet/Diffusion model files for FLUX or transformer models"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Unet path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePathService::GetPath("Unet");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePathService::GetPath("Unet");
		}

		DiffusionModelComponent& operator=(const DiffusionModelComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};


	struct HighNoiseDiffusionModelComponent : public BaseModelComponent {
		HighNoiseDiffusionModelComponent() {
			compName = "HighNoiseDiffusionModel";

			schema = {
				{"title", "High Noise UNet/Diffusion Model"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "High Noise UNet"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "High Noise UNet Models"},
							{"dialogDefaultPath", "Unet"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for high noise UNet/Diffusion model files (for video generation)"}
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
					options["dialogDefaultPath"] = Utils::FilePathService::GetPath("Unet");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePathService::GetPath("Unet");
		}

		HighNoiseDiffusionModelComponent& operator=(const HighNoiseDiffusionModelComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

} // namespace ECS