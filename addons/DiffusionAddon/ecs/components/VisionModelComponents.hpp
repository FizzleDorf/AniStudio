#pragma once

#include "BaseModelComponent.hpp"
#include "FilePaths.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	// Clip Vision 
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

	// Llm Vision Encoder
	struct LlmVisionComponent : public BaseModelComponent {
		LlmVisionComponent() {
			compName = "LlmVision";

			schema = {
				{"title", "LLM Vision Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "LLM Vision"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "LLM Vision Models"},
							{"dialogDefaultPath", Utils::FilePaths::GetInstance().GetPath("Encoder")},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for LLM vision encoder files"}
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

		LlmVisionComponent& operator=(const LlmVisionComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

}