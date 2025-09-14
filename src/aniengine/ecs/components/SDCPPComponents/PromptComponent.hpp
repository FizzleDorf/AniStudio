#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

	struct PromptComponent : public BaseComponent {
		PromptComponent() {
			compName = "Prompt";
			compCategory = "Sampling";

			schema = {
				{"title", "Prompt Settings"},
				{"type", "object"},
				{"propertyOrder", {"posPrompt", "negPrompt", "normalize_input"}},
				{"properties", {
					{"posPrompt", {
						{"type", "string"},
						{"title", "Positive"},
						{"ui:widget", "text_area"},
						{"ui:window_name", "Positive Prompt"},
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}},
					{"negPrompt", {
						{"type", "string"},
						{"title", "Negative"},
						{"ui:widget", "text_area"},
						{"ui:window_name", "Negative Prompt"},
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}},
					{"normalize_input", {
						{"type", "boolean"},
						{"title", "Normalize Input"},
						{"description", "Normalize token inputs for more consistent prompt processing. May improve prompt adherence."},
						{"ui:widget", "checkbox"}
					}}
				}}
			};
		}

		std::string posPrompt = "";
		std::string negPrompt = "";
		bool normalize_input = false;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"posPrompt", &posPrompt},
				{"negPrompt", &negPrompt},
				{"normalize_input", &normalize_input}
			};
		}

		PromptComponent& operator=(const PromptComponent& other) {
			if (this != &other) {
				posPrompt = other.posPrompt;
				negPrompt = other.negPrompt;
				normalize_input = other.normalize_input;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"posPrompt", posPrompt},
				{"negPrompt", negPrompt},
				{"normalize_input", normalize_input}
			};
			return j;
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

			if (componentData.contains("posPrompt")) {
				posPrompt = componentData["posPrompt"].get<std::string>();
			}
			if (componentData.contains("negPrompt")) {
				negPrompt = componentData["negPrompt"].get<std::string>();
			}
			if (componentData.contains("normalize_input")) {
				normalize_input = componentData["normalize_input"].get<bool>();
			}
		}
	};

} // namespace ECS