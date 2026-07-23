#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	struct BaseModelComponent : public BaseComponent {
		BaseModelComponent() {
			compName = "Models";
			compCategory = "Models";
		}
		std::string modelPath = "";
		std::string modelName = "";
		bool isModelLoaded = false;

		virtual void RefreshSchema() override {
			if (schema.contains("properties") && schema["properties"].contains("modelPath")) {
				auto& prop = schema["properties"]["modelPath"];
				if (prop.contains("ui:options")) {
					auto& options = prop["ui:options"];
					options["dialogDefaultPath"] = "";
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
		}

		virtual const char* GetDefaultDirectory() const {
			return compName;
		}
	};
}