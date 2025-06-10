/*
 * DoubleWidgets.hpp - Handles double* values from PropertyVariant
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_DOUBLE_WIDGET = "input_double";

	class DoubleWidgets {
	public:
		static bool RenderInputDouble(const std::string& label, double* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			double step = GetSchemaValue<double>(options, "step", 0.0);
			double step_fast = GetSchemaValue<double>(options, "step_fast", 0.0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.6f");
			return ImGui::InputDouble(label.c_str(), value, step, step_fast, format.c_str(), flags);
		}

		static bool RenderSliderDouble(const std::string& label, double* value, double min, double max, const nlohmann::json& options = {}) {
			std::string format = GetSchemaValue<std::string>(options, "format", "%.6f");
			ImGuiSliderFlags flags = GetSliderFlags(options);
			return ImGui::SliderScalar(label.c_str(), ImGuiDataType_Double, value, &min, &max, format.c_str(), flags);
		}

		static bool RenderDragDouble(const std::string& label, double* value, const nlohmann::json& options = {}) {
			double speed = GetSchemaValue<double>(options, "speed", 0.1);
			double min = GetSchemaValue<double>(options, "minimum", 0.0);
			double max = GetSchemaValue<double>(options, "maximum", 0.0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.6f");
			return ImGui::DragScalar(label.c_str(), ImGuiDataType_Double, value, static_cast<float>(speed), &min, &max, format.c_str());
		}

		static bool Render(const std::string& label, double* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "input_double") {
				return RenderInputDouble(label, value, schema);
			}
			else if (widgetType == "slider_double") {
				double min = GetSchemaValue<double>(schema, "minimum", 0.0);
				double max = GetSchemaValue<double>(schema, "maximum", 1.0);
				return RenderSliderDouble(label, value, min, max, schema);
			}
			else if (widgetType == "drag_double") {
				return RenderDragDouble(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for double property, defaulting to input_double" << std::endl;
				return RenderInputDouble(label, value, schema);
			}
		}
	};

} // namespace UISchema