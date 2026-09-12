#pragma once

#include "BaseModelComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	struct ClipGComponent : public BaseModelComponent {
		ClipGComponent() = default;

		const char* GetCompName() const override { return "ClipG"; }
		const char* GetCompCategory() const override { return "Models"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
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
							{"dialogDefaultPath", "encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-G text encoder files"}
						}}
					}}
				}}
			};
			return j;
		}

		ClipGComponent(const ClipGComponent& other) : BaseModelComponent(other) {}

		ClipGComponent& operator=(const ClipGComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	struct ClipLComponent : public BaseModelComponent {
		ClipLComponent() = default;

		const char* GetCompName() const override { return "ClipL"; }
		const char* GetCompCategory() const override { return "Models"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
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
							{"dialogDefaultPath", "encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for CLIP-L text encoder files"}
						}}
					}}
				}}
			};
			return j;
		}

		ClipLComponent(const ClipLComponent& other) : BaseModelComponent(other) {}

		ClipLComponent& operator=(const ClipLComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	struct T5XXLComponent : public BaseModelComponent {
		T5XXLComponent() = default;

		const char* GetCompName() const override { return "T5XXL"; }
		const char* GetCompCategory() const override { return "Models"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
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
							{"dialogDefaultPath", "encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for T5-XXL text encoder files for FLUX models"}
						}}
					}}
				}}
			};
			return j;
		}

		T5XXLComponent(const T5XXLComponent& other) : BaseModelComponent(other) {}

		T5XXLComponent& operator=(const T5XXLComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}
	};

	struct LlmEncoderComponent : public BaseModelComponent {
		LlmEncoderComponent() = default;

		const char* GetCompName() const override { return "LlmEncoder"; }
		const char* GetCompCategory() const override { return "Models"; }

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
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
							{"dialogDefaultPath", "encoder"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for LLM text encoder files"}
						}}
					}}
				}}
			};
			return j;
		}

		LlmEncoderComponent(const LlmEncoderComponent& other) : BaseModelComponent(other) {}

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