#pragma once

#include "BaseComponent.hpp"
#include <string>

namespace ECS {

	struct ClipSkipComponent : public ECS::BaseComponent {
		ClipSkipComponent() {
			compName = "ClipSkip";
			compCategory = "Advanced";
			schema = {
				{"title", "Clip Skip Settings"},
				{"type", "object"},
				{"properties", {
					{"clipSkip", {
						{"type", "integer"},
						{"title", "Clip Skip"},
						{"description", "Skip the last N layers of CLIP text encoder. Higher values can produce more artistic/abstract results but may reduce prompt adherence. Range: 0-12, typical: 1-2."},
						{"ui:widget", "input_int"},
						{"ui:options", {
							{"min", -5.0f},
							{"max", 15.0f},
							{"step", 1.0f}
						}}
					}}
				}}
			};
		}

		int clipSkip = 2;

		// Override the GetPropertyMap method
		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["clipSkip"] = &clipSkip;
			return properties;
		}

		ClipSkipComponent& operator=(const ClipSkipComponent& other) {
			if (this != &other) {
				clipSkip = other.clipSkip;
			}
			return *this;
		}

		nlohmann::json Serialize() const override {
			return { {compName,
					 {{"clipSkip", clipSkip}
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

			if (componentData.contains("clipSkip"))
				clipSkip = componentData["clipSkip"];
		}
	};

} // namespace ECS