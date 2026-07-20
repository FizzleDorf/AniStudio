#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {
	
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
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-G text encoder files"}
						}}
					}}
				}}
			};
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
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-L text encoder files"}
						}}
					}}
				}}
			};
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
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for T5-XXL text encoder files for FLUX models"}
						}}
					}}
				}}
			};
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

	// LLM Encoder
	struct LlmEncoderComponent : public BaseModelComponent {
		LlmEncoderComponent() {
			compName = "LlmEncoder";

			schema = {
				{"title", "LLM Text Encoder"},
				{"type", "object"},
				{"propertyOrder", {"modelPath"}},
				{"properties", {
					{"modelPath", {
						{"type", "string"},
						{"title", "LLM"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".safetensors,.ckpt,.pt,.gguf"},
							{"filterName", "LLM Models"},
							{"dialogDefaultPath", "Encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for LLM text encoder files"}
						}}
					}}
				}}
			};
		}

		LlmEncoderComponent& operator=(const LlmEncoderComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};
}