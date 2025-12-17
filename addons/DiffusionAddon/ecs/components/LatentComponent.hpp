#pragma once

#include "stable-diffusion.h"
#include "BaseComponent.hpp"
#include "DiffusionOptions.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace ECS {
	struct LatentComponent : public ECS::BaseComponent {
		LatentComponent() {
			compName = "Latent";
			compCategory = "Sampling";

			schema = {
				{"title", "Latent Settings"},
				{"type", "object"},
				{"propertyOrder", {"latentWidth", "latentHeight", "batchSize", "current_rng_type"}},
				{"properties", {
					{"latentWidth", {
						{"type", "integer"},
						{"title", "Width"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 64},
							{"step_fast", 64},
							{"min", 64},
							{"max", 2048}
						}}
					}},
					{"latentHeight", {
						{"type", "integer"},
						{"title", "Height"},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 64},
							{"step_fast", 64},
							{"min", 64},
							{"max", 2048}
						}}
					}},
					// {"batchSize", {
					// 	{"type", "integer"},
					// 	{"title", "Batch Size"},
					// 	{"ui:widget", "input_int"},
					// 	{"ui:options", {
					// 		{"step", 1},
					// 		{"step_fast", 4},
					// 		{"min", 1},
					// 		{"max", 16}
					// 	}}
					// }},
					{"current_rng_type", {
						{"type", "integer"},
						{"title", "RNG Type"},
						{"description", "Random number generator type. CUDA RNG provides different results than CPU RNG for the same seed."},
						{"ui:widget", "combo"},
						{"items", type_rng_items},
						{"itemCount", type_rng_item_count}
					}}
				}}
			};
		}

		// Core properties
		int latentWidth = 512;
		int latentHeight = 512;
		int batchSize = 1;
		rng_type_t current_rng_type = STD_DEFAULT_RNG;

		// Additional properties for DiffusionView to use
		bool useAspectRatio = false;
		bool isDivisibleBy64 = true;
		int longestSide = 768;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["latentWidth"] = &latentWidth;
			properties["latentHeight"] = &latentHeight;
			properties["batchSize"] = &batchSize;
			properties["current_rng_type"] = reinterpret_cast<int*>(&current_rng_type);
			return properties;
		}

		LatentComponent& operator=(const LatentComponent& other) {
			if (this != &other) {
				latentWidth = other.latentWidth;
				latentHeight = other.latentHeight;
				batchSize = other.batchSize;
				current_rng_type = other.current_rng_type;
				useAspectRatio = other.useAspectRatio;
				isDivisibleBy64 = other.isDivisibleBy64;
				longestSide = other.longestSide;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j[compName] = {
				{"latentWidth", latentWidth},
				{"latentHeight", latentHeight},
				{"batchSize", batchSize},
				{"current_rng_type", static_cast<int>(current_rng_type)},
				{"useAspectRatio", useAspectRatio},
				{"isDivisibleBy64", isDivisibleBy64},
				{"longestSide", longestSide}
			};
			return j;
		}

		void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;

			// First try to get data by component name
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			// If not found, look for any object that might contain our data
			else if (j.is_object() && j.size() == 1) {
				componentData = j.begin().value();
			}
			// Fallback: use the entire json
			else {
				componentData = j;
			}

			if (componentData.contains("latentWidth"))
				latentWidth = componentData["latentWidth"];
			if (componentData.contains("latentHeight"))
				latentHeight = componentData["latentHeight"];
			if (componentData.contains("batchSize"))
				batchSize = componentData["batchSize"];
			if (componentData.contains("current_rng_type"))
				current_rng_type = static_cast<rng_type_t>(componentData["current_rng_type"]);
			if (componentData.contains("useAspectRatio"))
				useAspectRatio = componentData["useAspectRatio"];
			if (componentData.contains("isDivisibleBy64"))
				isDivisibleBy64 = componentData["isDivisibleBy64"];
			if (componentData.contains("longestSide"))
				longestSide = componentData["longestSide"];
		}
	};
}