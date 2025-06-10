/*
 * BoolWidgets.hpp - Handles bool* values from PropertyVariant
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

namespace UISchema {

	static const std::string DEFAULT_BOOL_WIDGET = "checkbox";

	class BoolWidgets {
	public:
		static bool RenderCheckbox(const std::string& label, bool* value, const nlohmann::json& options = {}) {
			return ImGui::Checkbox(label.c_str(), value);
		}

		static bool Render(const std::string& label, bool* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "checkbox") {
				return RenderCheckbox(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for bool property, defaulting to checkbox" << std::endl;
				return RenderCheckbox(label, value, schema);
			}
		}
	};

} // namespace UISchema