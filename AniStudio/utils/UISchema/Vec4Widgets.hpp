/*
 * Vec4Widgets.hpp - Handles ImVec4* values from PropertyVariant
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_VEC4_WIDGET = "color";

	class Vec4Widgets {
	public:
		static bool RenderColorEdit(const std::string& label, ImVec4* value, const nlohmann::json& options = {}) {
			ImGuiColorEditFlags flags = GetColorEditFlags(options);
			return ImGui::ColorEdit4(label.c_str(), &value->x, flags);
		}

		static bool RenderVec4(const std::string& label, ImVec4* value, const nlohmann::json& options = {}) {
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			return ImGui::InputFloat4(label.c_str(), &value->x, format.c_str(), flags);
		}

		static bool Render(const std::string& label, ImVec4* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "color") {
				return RenderColorEdit(label, value, schema);
			}
			else if (widgetType == "vec4") {
				return RenderVec4(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for ImVec4 property, defaulting to color" << std::endl;
				return RenderColorEdit(label, value, schema);
			}
		}
	};

} // namespace UISchema