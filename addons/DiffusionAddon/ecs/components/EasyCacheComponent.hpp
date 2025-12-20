#pragma once

#include "BaseComponent.hpp"
#include "pch.h"
#include <string>

namespace ECS {

	struct EasyCacheComponent : public BaseComponent {
		EasyCacheComponent() {
			compName = "EasyCache";
			compCategory = "Performance";

			schema = {
				{"title", "EasyCache Settings"},
				{"type", "object"},
				{"description", "Cache management for faster generation"},
				{"propertyOrder", {"enabled", "reuse_threshold", "start_percent", "end_percent"}},
				{"properties", {
					{"enabled", {
						{"type", "boolean"},
						{"title", "Enable EasyCache"},
						{"description", "Enable caching mechanism for faster generation"},
						{"default", true},
						{"ui:widget", "checkbox"}
					}},
					{"reuse_threshold", {
						{"type", "number"},
						{"title", "Reuse Threshold"},
						{"description", "Threshold for reusing cached values (0.0-1.0)"},
						{"default", 0.8f},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"start_percent", {
						{"type", "number"},
						{"title", "Start Percentage"},
						{"description", "When to start caching (0.0-1.0)"},
						{"default", 0.0f},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}},
					{"end_percent", {
						{"type", "number"},
						{"title", "End Percentage"},
						{"description", "When to stop caching (0.0-1.0)"},
						{"default", 1.0f},
						{"ui:widget", "input_float"},
						{"ui:options", {
							{"step", 0.05f},
							{"step_fast", 0.1f},
							{"min", 0.0f},
							{"max", 1.0f}
						}}
					}}
				}}
			};
		}

		bool enabled = true;
		float reuse_threshold = 0.8f;
		float start_percent = 0.0f;
		float end_percent = 1.0f;

		std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			return {
				{"enabled", &enabled},
				{"reuse_threshold", &reuse_threshold},
				{"start_percent", &start_percent},
				{"end_percent", &end_percent}
			};
		}

		nlohmann::json Serialize() const override {
			return { {compName, {
				{"enabled", enabled},
				{"reuse_threshold", reuse_threshold},
				{"start_percent", start_percent},
				{"end_percent", end_percent}
			}} };
		}

		void Deserialize(const nlohmann::json& j) override {
			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				componentData = j;
			}

			if (componentData.contains("enabled")) enabled = componentData["enabled"].get<bool>();
			if (componentData.contains("reuse_threshold")) reuse_threshold = componentData["reuse_threshold"].get<float>();
			if (componentData.contains("start_percent")) start_percent = componentData["start_percent"].get<float>();
			if (componentData.contains("end_percent")) end_percent = componentData["end_percent"].get<float>();
		}
	};

} // namespace ECS