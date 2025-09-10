#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace ECS {
	struct LatentComponent : public ECS::BaseComponent {
		LatentComponent() {
			compName = "Latent";

			// Define the component schema - just the basic properties
			schema = {
				{"title", "Latent Settings"},
				{"type", "object"},
				{"propertyOrder", {"latentWidth", "latentHeight", "batchSize"}},
				{"properties", {
					{"latentWidth", {
						{"type", "integer"},
						{"title", "Width"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 8},
							{"step_fast", 32},
							{"min", 64},
							{"max", 2048}
						}}
					}},
					{"latentHeight", {
						{"type", "integer"},
						{"title", "Height"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 8},
							{"step_fast", 32},
							{"min", 64},
							{"max", 2048}
						}}
					}},
					{"batchSize", {
						{"type", "integer"},
						{"title", "Batch Size"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 4},
							{"min", 1},
							{"max", 16}
						}}
					}}
				}}
			};
		}

		// Core properties
		int latentWidth = 512;
		int latentHeight = 512;
		int batchSize = 1;

		// Additional properties for DiffusionView to use
		bool useAspectRatio = false;
		bool isDivisibleBy64 = true;
		int longestSide = 768;

		// Override the GetPropertyMap method - FIXED: Use same pattern as SamplerComponent
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["latentWidth"] = &latentWidth;
			properties["latentHeight"] = &latentHeight;
			properties["batchSize"] = &batchSize;
			return properties;
		}

		// Serialize the component to JSON
		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"latentWidth", latentWidth},
				{"latentHeight", latentHeight},
				{"batchSize", batchSize},
				{"useAspectRatio", useAspectRatio},
				{"isDivisibleBy64", isDivisibleBy64},
				{"longestSide", longestSide}
			};
			return j;
		}

		// Deserialize the component from JSON
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

			if (componentData.contains("latentWidth"))
				latentWidth = componentData["latentWidth"];
			if (componentData.contains("latentHeight"))
				latentHeight = componentData["latentHeight"];
			if (componentData.contains("batchSize"))
				batchSize = componentData["batchSize"];
			if (componentData.contains("useAspectRatio"))
				useAspectRatio = componentData["useAspectRatio"];
			if (componentData.contains("isDivisibleBy64"))
				isDivisibleBy64 = componentData["isDivisibleBy64"];
			if (componentData.contains("longestSide"))
				longestSide = componentData["longestSide"];
		}
	};
}