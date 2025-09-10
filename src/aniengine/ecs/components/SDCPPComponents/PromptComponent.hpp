#pragma once

#include "BaseComponent.hpp"
#include "stable-diffusion.h"
#include <string>

namespace ECS {

	struct PromptComponent : public BaseComponent {
		PromptComponent() {
			compName = "Prompt";

			schema = {
				{"title", "Prompt Settings"},
				{"type", "object"},
				{"propertyOrder", {"posPrompt", "negPrompt"}},
				{"ui:separate_windows", true},  // Enable separate window mode
				{"properties", {
					{"posPrompt", {
						{"type", "string"},
						{"title", "Positive"},
						{"ui:widget", "text_editor"},
						{"ui:window", true},
						{"ui:window_name", "Positive Prompt"},  // Window name
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}},
					{"negPrompt", {
						{"type", "string"},
						{"title", "Negative"},
						{"ui:widget", "text_editor"},
						{"ui:window", true},  // Create separate window for this property
						{"ui:window_name", "Negative Prompt"},  // Window name
						{"ui:options", {
							{"maxLength", 8192},
							{"showMenuBar", true}
						}}
					}}
				}}
			};
		}

		std::string posPrompt = "";
		std::string negPrompt = "";

		// CRITICAL: Override GetPropertyMap to return the actual property pointers
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"posPrompt", &posPrompt},
				{"negPrompt", &negPrompt}
			};
		}

		PromptComponent& operator=(const PromptComponent& other) {
			if (this != &other) {
				posPrompt = other.posPrompt;
				negPrompt = other.negPrompt;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"posPrompt", posPrompt},
				{"negPrompt", negPrompt}
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
		}

	};

} // namespace ECS