#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	// Base class for any loaded models
	struct BaseModelComponent : public BaseComponent {
		BaseModelComponent() {
			compName = "BaseModelComponent";
			compCategory = "Models";
		}
		std::string modelPath = "";
		std::string modelName = "";
		bool isModelLoaded = false;

		virtual void RefreshSchema() override {
			// Update paths in schema to reflect current FilePaths settings
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = GetDefaultDirectory().string();
				}
			}
		}

		virtual nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath}
			}} };
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelName", &modelName},
				{"modelPath", &modelPath}
			};
		}

		virtual void Deserialize(const nlohmann::json& j) override {
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

			if (componentData.contains("modelName"))
				modelName = componentData["modelName"];
			if (componentData.contains("modelPath"))
				modelPath = componentData["modelPath"];

			// Auto-populate missing fields
			UpdatePathsFromName();
		}

		// Helper to update paths - only as fallback when data is incomplete
		virtual void UpdatePathsFromName() {
			// just clear everything if there is no path
			if (modelPath.empty()) {
				modelName.clear();
				return;
			}

			// If we have a valid full path, just ensure modelName is set
			if (!modelPath.empty() && std::filesystem::exists(modelPath)) {
				if (modelName.empty()) {
					std::filesystem::path pathObj(modelPath);
					modelName = pathObj.filename().string();
				}
				return;
			}

			// If modelName contains a full path, split it properly
			if (!modelName.empty()) {
				std::filesystem::path namePath(modelName);
				if (namePath.is_absolute()) {
					modelPath = modelName;
					modelName = namePath.filename().string();
					return;
				}
			}

			// Fallback: construct path from name using default directory
			if (!modelName.empty() && modelPath.empty()) {
				std::filesystem::path fallbackPath = GetDefaultDirectory() / modelName;
				modelPath = fallbackPath.string();
			}
		}

		// Get the appropriate default directory for this model type
		virtual std::filesystem::path GetDefaultDirectory() const {
			return Utils::FilePaths::GetInstance().GetPath("Checkpoint"); // Base class default
		}
	};

	// Packaged Checkpoint loader (sd1.5 and sdxl with vae and encoders)
	struct ModelComponent : public BaseModelComponent {
		ModelComponent() {
			compName = "Model";
			schema = {
				{"title", "Checkpoint Model"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Model"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "Checkpoint Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Checkpoint")},
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Checkpoint");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Checkpoint");
		}

		ModelComponent& operator=(const ModelComponent& other) {
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
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Unet")},
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Unet");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Unet");
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

	// Clip G Encoder
	struct ClipGComponent : public BaseModelComponent {
		ClipGComponent() {
			compName = "ClipG";
			schema = {
				{"title", "CLIP-G Text Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "CLIP-G"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "CLIP Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-G text encoder files"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Encoder path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		ClipGComponent& operator=(const ClipGComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Clip L Encoder
	struct ClipLComponent : public BaseModelComponent {
		ClipLComponent() {
			compName = "ClipL";

			schema = {
				{"title", "CLIP-L Text Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "CLIP-L"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "CLIP Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-L text encoder files"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Encoder path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		ClipLComponent& operator=(const ClipLComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// T5 Encoder
	struct T5XXLComponent : public BaseModelComponent {
		T5XXLComponent() {
			compName = "T5XXL";

			schema = {
				{"title", "T5-XXL Text Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "T5-XXL"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "T5 Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for T5-XXL text encoder files for FLUX models"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Encoder path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		T5XXLComponent& operator=(const T5XXLComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Clip Vision Component (for image encoding)
	struct ClipVisionComponent : public BaseModelComponent {
		ClipVisionComponent() {
			compName = "ClipVision";

			schema = {
				{"title", "CLIP Vision Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "CLIP Vision"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "CLIP Vision Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP Vision encoder files (for img2img, etc.)"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Encoder path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		ClipVisionComponent& operator=(const ClipVisionComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	struct Qwen2VLComponent : public BaseModelComponent {
		Qwen2VLComponent() {
			compName = "Qwen2VL";

			schema = {
				{"title", "Qwen2VL Text Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Qwen2VL"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "Qwen2VL Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for Qwen2VL text encoder files"}
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		Qwen2VLComponent& operator=(const Qwen2VLComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	// Qwen2VL Vision Encoder Component
	struct Qwen2VLVisionComponent : public BaseModelComponent {
		Qwen2VLVisionComponent() {
			compName = "Qwen2VLVision";

			schema = {
				{"title", "Qwen2VL Vision Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Qwen2VL Vision"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "Qwen2VL Vision Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for Qwen2VL vision encoder files"}
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Encoder");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Encoder");
		}

		Qwen2VLVisionComponent& operator=(const Qwen2VLVisionComponent& other) {
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
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Unet")},
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Unet");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Unet");
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
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("PhotoMaker")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for Photo Maker identity embedding/face swapping models"}
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("PhotoMaker");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("PhotoMaker");
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

	// VAE Component
	struct VaeComponent : public BaseModelComponent {
		VaeComponent() {
			compName = "Vae";

			schema = {
				{"title", "VAE Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "isTiled", "keep_vae_on_cpu", "vae_decode_only"}},
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
			// Update the default path in the schema to current Vae path
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
			// Update the default path in the schema to current Vae path
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

	// Low Rank Attention Model loader
	struct LoraComponent : public ECS::BaseModelComponent {
		LoraComponent() {
			compName = "Lora";
			modelPath = Utils::FilePaths::GetInstance().GetPath("Lora");
			schema = {
				{"title", "LoRA Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath", "loraStrength", "loraClipStrength"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "LoRA"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt"},
							{"filterName", "LoRA Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Lora")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for LoRA adaptation files"}
						}}
					}},
					{"loraStrength", {
						{"type", "number"},
						{"title", "UNet Strength"},
						{"description", "Strength of LoRA effect on the UNet (image generation). Higher values increase the LoRA's influence."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.2f},
							{"format", "%.1f"},
							{"min", -2.0f},
							{"max", 2.0f}
						}}
					}},
					{"loraClipStrength", {
						{"type", "number"},
						{"title", "CLIP Strength"},
						{"description", "Strength of LoRA effect on text encoder (prompt understanding). Usually kept same as UNet strength."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.1f},
							{"step_fast", 0.2f},
							{"format", "%.1f"},
							{"min", -2.0f},
							{"max", 2.0f}
						}}
					}}
				}}
			};
		}

		float loraStrength = 1.0f;
		float loraClipStrength = 1.0f;

		void RefreshSchema() override {
			// Update the default path in the schema to current Lora path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Lora");
				}
			}
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"loraStrength", &loraStrength},
				{"loraClipStrength", &loraClipStrength}
			};
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Lora");
		}

		LoraComponent& operator=(const LoraComponent& other) {
			if (this != &other) {
				modelName = other.modelName;
				modelPath = other.modelPath;
				isModelLoaded = other.isModelLoaded;
				loraStrength = other.loraStrength;
				loraClipStrength = other.loraClipStrength;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath},
				{"loraStrength", loraStrength},
				{"loraClipStrength", loraClipStrength}
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

			if (componentData.contains("loraStrength"))
				loraStrength = componentData["loraStrength"];
			if (componentData.contains("loraClipStrength"))
				loraClipStrength = componentData["loraClipStrength"];
		}
	};

	// ControlNet Component
	struct ControlNetComponent : public ECS::BaseModelComponent {
		ControlNetComponent() {
			compName = "Controlnet";

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
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("ControlNet")},
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

		void RefreshSchema() override {
			// Update the default path in the schema to current ControlNet path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("ControlNet");
				}
			}
		}

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

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("ControlNet");
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

	// Upscale models
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
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Upscale")},
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
					{"preserveAspectRatio", {
						{"type", "boolean"},
						{"title", "Preserve Aspect Ratio"},
						{"description", "Maintain the original image proportions during upscaling to prevent distortion."},
						{"ui:widget", "checkbox"}
					}}
				}}
			};
		}

		uint32_t upscaleFactor = 2;
		bool preserveAspectRatio = true;

		void RefreshSchema() override {
			// Update the default path in the schema to current Upscale path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Upscale");
				}
			}
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName},
				{"upscaleFactor", reinterpret_cast<int*>(&upscaleFactor)},
				{"preserveAspectRatio", &preserveAspectRatio}
			};
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Upscale");
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

	// Embedding
	struct EmbeddingComponent : public BaseModelComponent {
		EmbeddingComponent() {
			compName = "Embedding";

			schema = {
				{"title", "Text Embedding"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Embedding"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.bin"},
							{"filterName", "Embedding Files"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Embed")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for textual inversion embedding files"}
						}}
					}}
				}}
			};
		}

		void RefreshSchema() override {
			// Update the default path in the schema to current Embed path
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Embed");
				}
			}
		}

		std::filesystem::path GetDefaultDirectory() const override {
			return Utils::FilePaths::GetInstance().GetPath("Embed");
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