#pragma once

#include "BaseModelComponent.hpp"
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

		HighNoiseDiffusionModelComponent& operator=(const HighNoiseDiffusionModelComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	struct UncondDiffusionModelComponent : public BaseModelComponent {
		UncondDiffusionModelComponent() {
			compName = "UncondDiffusionModel";
			schema = {
				{"title", "Unconditional Diffusion Model"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Uncond UNet"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "UNet Models"},
							{"dialogDefaultPath", "Unet"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for unconditional diffusion model files"}
						}}
					}}
				}}
			};
		}

		UncondDiffusionModelComponent& operator=(const UncondDiffusionModelComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Audio VAE Component
	struct AudioVAEComponent : public BaseModelComponent {
		AudioVAEComponent() {
			compName = "AudioVAE";
			schema = {
				{"title", "Audio VAE Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Audio VAE"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "Audio VAE Models"},
							{"dialogDefaultPath", "Vae"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for audio VAE model files (for audio/video models)"}
						}}
					}}
				}}
			};
		}

		AudioVAEComponent& operator=(const AudioVAEComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Embeddings Connectors Component (for models like flux with special connectors)
	struct EmbeddingComponent : public BaseModelComponent {
		EmbeddingComponent() {
			compName = "Embedding";
			schema = {
				{"title", "Embeddings Connectors"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Connectors Path"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "Connector Models"},
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for embeddings connector model files"}
						}}
					}}
				}}
			};
		}

		EmbeddingComponent& operator=(const EmbeddingComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

} // namespace ECS