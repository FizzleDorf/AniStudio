#pragma once

#include "BaseModelComponent.hpp"
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
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP Vision encoder files (for img2img, etc.)"}
						}}
					}}
				}}
			};
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
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for LLM vision encoder files"}
						}}
					}}
				}}
			};
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