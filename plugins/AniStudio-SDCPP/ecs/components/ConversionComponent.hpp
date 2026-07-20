#pragma once

#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <string>

namespace ECS {

	struct ConversionComponent : public BaseComponent {
		ConversionComponent() {
			compName = "Conversion";
			compCategory = "Tools";
			schema = {
				{"title", "Model Conversion Settings"},
				{"type", "object"},
				{"properties", {
					{"tensorTypeRules", {
						{"type", "string"},
						{"title", "Tensor Type Rules"},
						{"description", "Optional rules for tensor type conversion"}
					}},
					{"convertName", {
						{"type", "boolean"},
						{"title", "Convert Layer Names"},
						{"description", "Whether to convert layer names during conversion"},
						{"default", true}
					}}
				}}
			};
		}

		std::string tensorTypeRules = "";
		bool convertName = true;

		virtual nlohmann::json Serialize() const override {
			return { {compName, {
				{"tensorTypeRules", tensorTypeRules},
				{"convertName", convertName}
			}} };
		}

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"tensorTypeRules", &tensorTypeRules},
				{"convertName", &convertName}
			};
		}

		virtual void Deserialize(const nlohmann::json& j) override {
			if (j.contains(compName)) {
				auto compData = j.at(compName);
				if (compData.contains("tensorTypeRules"))
					tensorTypeRules = compData["tensorTypeRules"];
				if (compData.contains("convertName"))
					convertName = compData["convertName"];
			}
		}
	};

} // namespace ECS