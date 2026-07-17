#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

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
		virtual const char* GetDefaultDirectory() const {
			return compCategory;
		}
	};
}