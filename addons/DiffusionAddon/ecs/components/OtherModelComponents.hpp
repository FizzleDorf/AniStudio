#pragma once

#include "BaseModelComponent.hpp"
#include "FilePaths.hpp"
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

	// LoRa Component
	struct LoraComponent : public ECS::BaseModelComponent {
		LoraComponent() {
			compName = "Lora";
			modelPath = Utils::FilePaths::GetInstance().GetPath("Lora");
			schema = {
				{"title", "LoRA Settings"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "LoRA Directory"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "directory"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Lora")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for root LoRA directory"}
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
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Lora");
				}
			}
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName}
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
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseModelComponent::Deserialize(j);
		}
	};

	// Embedding Component
	struct EmbeddingComponent : public BaseModelComponent {
		EmbeddingComponent() {
			compName = "Embedding";
			modelPath = Utils::FilePaths::GetInstance().GetPath("Embed");

			schema = {
				{"title", "Text Embedding"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "Embedding Directory"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "directory"},
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
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = Utils::FilePaths::GetInstance().GetPath("Embed");
				}
			}
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelPath", &modelPath},
				{"modelName", &modelName}
			};
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

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"modelName", modelName},
				{"modelPath", modelPath}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseModelComponent::Deserialize(j);
		}
	};

}