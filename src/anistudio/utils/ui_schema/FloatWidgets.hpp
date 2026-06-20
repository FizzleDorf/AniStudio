#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_FLOAT_WIDGET = "input_float";

	class FloatWidgets {
	public:
		static bool RenderInputFloat(const std::string& label, float* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			float step = GetSchemaValue<float>(options, "step", 0.0f);
			float step_fast = GetSchemaValue<float>(options, "step_fast", 0.0f);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			return ImGui::InputFloat(label.c_str(), value, step, step_fast, format.c_str(), flags);
		}

		static bool RenderSliderFloat(const std::string& label, float* value, float min, float max, const nlohmann::json& options = {}) {
			ImGuiSliderFlags flags = GetSliderFlags(options);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			return ImGui::SliderFloat(label.c_str(), value, min, max, format.c_str(), flags);
		}

		static bool RenderDragFloat(const std::string& label, float* value, const nlohmann::json& options = {}) {
			float speed = GetSchemaValue<float>(options, "speed", 0.1f);
			// FIXED: Use "min" and "max" instead of "minimum" and "maximum"
			float min = GetSchemaValue<float>(options, "min", 0.0f);
			float max = GetSchemaValue<float>(options, "max", 0.0f);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			return ImGui::DragFloat(label.c_str(), value, speed, min, max, format.c_str());
		}

		static bool Render(const std::string& label, float* value, const std::string& widgetType, const nlohmann::json& schema) {
			// FIXED: Extract options from "ui:options" path
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
				options = schema["ui:options"];
			}

			if (widgetType == "input_float") {
				return RenderInputFloat(label, value, options);
			}
			else if (widgetType == "slider_float") {
				float min = GetSchemaValue<float>(options, "min", 0.0f);
				float max = GetSchemaValue<float>(options, "max", 1.0f);
				// Also check schema root level for min/max (fallback)
				if (min == 0.0f && max == 1.0f) {
					min = GetSchemaValue<float>(schema, "minimum", 0.0f);
					max = GetSchemaValue<float>(schema, "maximum", 1.0f);
				}
				return RenderSliderFloat(label, value, min, max, options);
			}
			else if (widgetType == "drag_float") {
				return RenderDragFloat(label, value, options);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for float property, defaulting to input_float" << std::endl;
				return RenderInputFloat(label, value, options);
			}
		}
	};

} // namespace UISchema