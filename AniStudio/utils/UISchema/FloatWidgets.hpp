/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

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
			float min = GetSchemaValue<float>(options, "minimum", 0.0f);
			float max = GetSchemaValue<float>(options, "maximum", 0.0f);
			std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
			return ImGui::DragFloat(label.c_str(), value, speed, min, max, format.c_str());
		}

		static bool Render(const std::string& label, float* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "input_float") {
				return RenderInputFloat(label, value, schema);
			}
			else if (widgetType == "slider_float") {
				float min = GetSchemaValue<float>(schema, "minimum", 0.0f);
				float max = GetSchemaValue<float>(schema, "maximum", 1.0f);
				return RenderSliderFloat(label, value, min, max, schema);
			}
			else if (widgetType == "drag_float") {
				return RenderDragFloat(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for float property, defaulting to input_float" << std::endl;
				return RenderInputFloat(label, value, schema);
			}
		}
	};

} // namespace UISchema