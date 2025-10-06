#pragma once

#include "BaseComponent.hpp"
#include <string>
#include <vector>

namespace ECS {

	struct PhotoMakerComponent : public ECS::BaseComponent {
		PhotoMakerComponent() {
			compName = "PhotoMaker";
			compCategory = "Image";

			schema = {
				{"title", "PhotoMaker Settings"},
				{"type", "object"},
				{"propertyOrder", {"id_images_count", "id_embed_path", "style_strength"}},
				{"properties", {
					{"id_images_count", {
						{"type", "integer"},
						{"title", "ID Images Count"},
						{"description", "Number of identity reference images to use. More images can improve identity consistency but increase processing time."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"step", 1},
							{"step_fast", 1},
							{"min", 0},
							{"max", 10}
						}}
					}},
					{"id_embed_path", {
						{"type", "string"},
						{"title", "ID Embedding Path"},
						{"description", "Path to the pre-computed identity embedding file. If provided, skips the encoding step and uses cached identity features."},
						{"ui:widget", "input_text"},
						{"ui:options", {
							{"placeholder", "path/to/id_embedding.safetensors"}
						}}
					}},
					{"style_strength", {
						{"type", "number"},
						{"title", "Style Strength"},
						{"description", "Controls the balance between identity preservation and style/prompt adherence. 0.0 = full prompt adherence, 1.0 = maximum identity preservation. Recommended: 0.2-0.8"},
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

		// sd_pm_params_t members
		// Note: id_images is handled separately as it's an array of sd_image_t
		// Store paths to images instead of raw sd_image_t data
		std::vector<std::string> id_image_paths;
		int id_images_count = 0;
		std::string id_embed_path;
		float style_strength = 0.5f;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["id_images_count"] = &id_images_count;
			properties["id_embed_path"] = &id_embed_path;
			properties["style_strength"] = &style_strength;
			return properties;
		}

		PhotoMakerComponent& operator=(const PhotoMakerComponent& other) {
			if (this != &other) {
				id_image_paths = other.id_image_paths;
				id_images_count = other.id_images_count;
				id_embed_path = other.id_embed_path;
				style_strength = other.style_strength;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			nlohmann::json imagePathsArray = nlohmann::json::array();
			for (const auto& path : id_image_paths) {
				imagePathsArray.push_back(path);
			}

			return { {compName, {
				{"id_image_paths", imagePathsArray},
				{"id_images_count", id_images_count},
				{"id_embed_path", id_embed_path},
				{"style_strength", style_strength}
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

			if (componentData.contains("id_images_count"))
				id_images_count = componentData["id_images_count"];
			if (componentData.contains("id_embed_path"))
				id_embed_path = componentData["id_embed_path"].get<std::string>();
			if (componentData.contains("style_strength"))
				style_strength = componentData["style_strength"];

			// Deserialize image paths array
			if (componentData.contains("id_image_paths") && componentData["id_image_paths"].is_array()) {
				id_image_paths.clear();
				auto imagePathsArray = componentData["id_image_paths"];
				for (const auto& path : imagePathsArray) {
					id_image_paths.push_back(path.get<std::string>());
				}
				// Update count to match actual paths
				id_images_count = id_image_paths.size();
			}
		}

		// Helper method to add an ID image path
		void AddIdImagePath(const std::string& path) {
			id_image_paths.push_back(path);
			id_images_count = id_image_paths.size();
		}

		// Helper method to remove an ID image path
		void RemoveIdImagePath(size_t index) {
			if (index < id_image_paths.size()) {
				id_image_paths.erase(id_image_paths.begin() + index);
				id_images_count = id_image_paths.size();
			}
		}

		// Helper method to clear all ID images
		void ClearIdImages() {
			id_image_paths.clear();
			id_images_count = 0;
		}
	};

} // namespace ECS