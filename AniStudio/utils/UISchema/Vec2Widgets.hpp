/*
 * Vec2Widgets.hpp - Handles ImVec2* values from PropertyVariant
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_VEC2_WIDGET = "vec2";

	class Vec2Widgets {
	public:
		static bool RenderVec2(const std::string& label, ImVec2* value, const nlohmann::json& options = {}) {
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			return ImGui::InputFloat2(label.c_str(), &value->x, format.c_str(), flags);
		}

		static bool Render(const std::string& label, ImVec2* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "vec2") {
				return RenderVec2(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for ImVec2 property, defaulting to vec2" << std::endl;
				return RenderVec2(label, value, schema);
			}
		}
	};

} // namespace UISchema