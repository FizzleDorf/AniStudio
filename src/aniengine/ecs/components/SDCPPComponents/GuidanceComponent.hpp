#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

	struct GuidanceComponent : public ECS::BaseComponent {
		GuidanceComponent() {
			compName = "Guidance";
			compCategory = "Sampling";

			// Define the component schema WITHOUT table layout + with tooltips
			schema = {
				{"title", "Guidance Settings"},
				{"type", "object"},
				{"propertyOrder", {"guidance", "eta"}},
				// REMOVED: {"ui:table", {...}} section entirely
				{"properties", {
					{"guidance", {
						{"type", "number"},
						{"title", "Guidance Scale"},
						{"description", "Controls how closely the model follows the prompt. Higher values increase prompt adherence but may reduce image quality and creativity. Typical range: 1-20, recommended: 7-12."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 1.0f},
							{"max", 30.0f}
						}}
					}},
					{"eta", {
						{"type", "number"},
						{"title", "ETA"},
						{"description", "Eta parameter for DDIM scheduler. Controls the amount of noise added during sampling. 0.0 = deterministic (DDIM), 1.0 = stochastic (DDPM). Higher values add more randomness."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}}
				}}
			};
		}

		float guidance = 2.0f;
		float eta = 0.0f;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["guidance"] = &guidance;
			properties["eta"] = &eta;
			return properties;
		}

		GuidanceComponent& operator=(const GuidanceComponent& other) {
			if (this != &other) {
				guidance = other.guidance;
				eta = other.eta;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName,
					 {{"guidance", guidance},
					  {"eta", eta}
					 }} };
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

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

			if (componentData.contains("guidance"))
				guidance = componentData["guidance"];
			if (componentData.contains("eta"))
				eta = componentData["eta"];
		}
	};

} // namespace ECS