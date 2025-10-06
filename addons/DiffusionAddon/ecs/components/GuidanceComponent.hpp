#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

	struct GuidanceComponent : public ECS::BaseComponent {
		GuidanceComponent() {
			compName = "Guidance";
			compCategory = "Sampling";

			schema = {
				{"title", "Guidance Settings"},
				{"type", "object"},
				{"propertyOrder", {"distilled_guidance", "txt_cfg", "img_cfg", "eta"}},
				{"properties", {
					{"distilled_guidance", {
						{"type", "number"},
						{"title", "Distilled Guidance"},
						{"description", "Guidance scale for distilled models. Controls the strength of guidance from the distilled model. Typical range: 1-20."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 1.0f},
							{"max", 30.0f}
						}}
					}},
					{"txt_cfg", {
						{"type", "number"},
						{"title", "Text CFG"},
						{"description", "Classifier-Free Guidance scale for text conditioning. Controls how closely the model follows the text prompt. Higher values increase prompt adherence. Typical range: 1-20, recommended: 7-12."},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.5f},
							{"format", "%.2f"},
							{"min", 1.0f},
							{"max", 30.0f}
						}}
					}},
					{"img_cfg", {
						{"type", "number"},
						{"title", "Image CFG"},
						{"description", "Classifier-Free Guidance scale for image conditioning. Controls adherence to reference images when doing image-to-image generation. Typical range: 1-20."},
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

		// sd_guidance_params_t members
		float txt_cfg = 7.0f;
		float img_cfg = 1.5f;
		float distilled_guidance = 3.5f;

		// eta (part of sd_sample_params_t but grouped here for UI convenience)
		float eta = 0.0f;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["distilled_guidance"] = &distilled_guidance;
			properties["img_cfg"] = &img_cfg;
			properties["txt_cfg"] = &txt_cfg;
			properties["eta"] = &eta;
			return properties;
		}

		GuidanceComponent& operator=(const GuidanceComponent& other) {
			if (this != &other) {
				txt_cfg = other.txt_cfg;
				img_cfg = other.img_cfg;
				distilled_guidance = other.distilled_guidance;
				eta = other.eta;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"txt_cfg", txt_cfg},
				{"img_cfg", img_cfg},
				{"distilled_guidance", distilled_guidance},
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

			if (componentData.contains("txt_cfg"))
				txt_cfg = componentData["txt_cfg"];
			if (componentData.contains("img_cfg"))
				img_cfg = componentData["img_cfg"];
			if (componentData.contains("distilled_guidance"))
				distilled_guidance = componentData["distilled_guidance"];
			if (componentData.contains("eta"))
				eta = componentData["eta"];
		}
	};

} // namespace ECS