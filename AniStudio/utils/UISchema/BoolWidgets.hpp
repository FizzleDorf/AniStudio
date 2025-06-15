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