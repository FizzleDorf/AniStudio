#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	struct BaseModelComponent : public BaseComponent {
		std::string modelPath = "";
		std::string modelName = "";
		bool isModelLoaded = false;

		BaseModelComponent() = default;

		const nlohmann::json& GetSchema() const override {
			static const nlohmann::json j = {
				{"title", "Base Model"},
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
							{"dialogDefaultPath", "checkpoint"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for checkpoint model files (.safetensors, .ckpt, .pt, .gguf)"}
						}}
					}}
				}}
			};
			return j;
		}

		BaseModelComponent(const BaseModelComponent& other) : BaseComponent(other) {
			modelPath = other.modelPath;
			modelName = other.modelName;
			isModelLoaded = other.isModelLoaded;
		}

		BaseModelComponent& operator=(const BaseModelComponent& other) {
			if (this != &other) {
				modelPath = other.modelPath;
				modelName = other.modelName;
				isModelLoaded = other.isModelLoaded;
			}
			return *this;
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"modelName", &modelName},
				{"modelPath", &modelPath}
			};
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[GetCompName()] = {
				{"modelName", modelName},
				{"modelPath", modelPath}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			const char* key = GetCompName();
			nlohmann::json componentData;

			if (j.contains(key)) {
				componentData = j.at(key);
			}
			else {
				for (auto it = j.begin(); it != j.end(); ++it) {
					if (it.key() == key) {
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
		}

		virtual const char* GetDefaultDirectory() const {
			return GetCompName();
		}
	};
}