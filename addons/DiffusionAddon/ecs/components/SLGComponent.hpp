#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

	struct SLGComponent : public ECS::BaseComponent {
		SLGComponent() {
			compName = "SLG";
			compCategory = "Sampling";

			schema = {
				{"title", "Shifted Layer Guidance Settings"},
				{"type", "object"},
				{"propertyOrder", {"layers", "layer_count", "layer_start", "layer_end", "scale"}},
				{"properties", {
					{"layers", {
						{"type", "integer"},
						{"title", "Layers"},
						{"description", "Array of layer indices to apply shifted guidance to. Controls which layers of the model are affected by the guidance shift."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 5},
							{"min", 0},
							{"max", 30}
						}}
					}},
					{"layer_count", {
						{"type", "integer"},
						{"title", "Layer Count"},
						{"description", "Number of layers in the layers array. Determines how many model layers will use shifted guidance."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 5},
							{"min", 1},
							{"max", 30}
						}}
					}},
					{"layer_start", {
						{"type", "number"},
						{"title", "Layer Start"},
						{"description", "Starting point for layer guidance application. Defines the beginning threshold for applying guidance shifts across layers."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"layer_end", {
						{"type", "number"},
						{"title", "Layer End"},
						{"description", "Ending point for layer guidance application. Defines the final threshold for applying guidance shifts across layers."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"scale", {
						{"type", "number"},
						{"title", "Scale"},
						{"description", "Overall scaling factor for shifted layer guidance. Higher values increase the strength of the guidance effect on selected layers."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 0.0f},
							{"max", 10.0f}
						}}
					}}
				}}
			};
		}

		// sd_slg_params_t members
		int* layers = nullptr;
		size_t layer_count = 0;
		float layer_start = 0.0f;
		float layer_end = 1.0f;
		float scale = 1.0f;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["layers"] = layers;
			properties["layer_count"] = &layer_count;
			properties["layer_start"] = &layer_start;
			properties["layer_end"] = &layer_end;
			properties["scale"] = &scale;
			return properties;
		}

		SLGComponent& operator=(const SLGComponent& other) {
			if (this != &other) {
				layer_count = other.layer_count;
				layer_start = other.layer_start;
				layer_end = other.layer_end;
				scale = other.scale;

				// Deep copy the layers array
				if (layers != nullptr) {
					delete[] layers;
					layers = nullptr;
				}
				if (other.layers != nullptr && other.layer_count > 0) {
					layers = new int[other.layer_count];
					memcpy(layers, other.layers, other.layer_count * sizeof(int));
				}
			}
			return *this;
		}

		~SLGComponent() {
			if (layers != nullptr) {
				delete[] layers;
				layers = nullptr;
			}
		}

		nlohmann::json Serialize() const override {
			nlohmann::json layersArray = nlohmann::json::array();
			if (layers != nullptr && layer_count > 0) {
				for (size_t i = 0; i < layer_count; i++) {
					layersArray.push_back(layers[i]);
				}
			}

			return { {compName, {
				{"layers", layersArray},
				{"layer_count", layer_count},
				{"layer_start", layer_start},
				{"layer_end", layer_end},
				{"scale", scale}
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

			if (componentData.contains("layer_count"))
				layer_count = componentData["layer_count"];
			if (componentData.contains("layer_start"))
				layer_start = componentData["layer_start"];
			if (componentData.contains("layer_end"))
				layer_end = componentData["layer_end"];
			if (componentData.contains("scale"))
				scale = componentData["scale"];

			// Deserialize layers array
			if (componentData.contains("layers") && componentData["layers"].is_array()) {
				if (layers != nullptr) {
					delete[] layers;
					layers = nullptr;
				}

				auto layersArray = componentData["layers"];
				layer_count = layersArray.size();

				if (layer_count > 0) {
					layers = new int[layer_count];
					for (size_t i = 0; i < layer_count; i++) {
						layers[i] = layersArray[i];
					}
				}
			}
		}
	};

} // namespace ECS