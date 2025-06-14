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
#include <vector>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_STRING_ARRAY_WIDGET = "list";

	class StringArrayWidgets {
	public:
		static bool RenderList(const std::string& label, std::vector<std::string>* value, const nlohmann::json& options = {}) {
			bool modified = false;

			if (!label.empty()) {
				ImGui::Text("%s", label.c_str());
			}

			for (size_t i = 0; i < value->size(); ++i) {
				ImGui::PushID(static_cast<int>(i));

				char buffer[256];
				strncpy(buffer, (*value)[i].c_str(), sizeof(buffer) - 1);
				buffer[sizeof(buffer) - 1] = '\0';

				if (ImGui::InputText("##item", buffer, sizeof(buffer))) {
					(*value)[i] = buffer;
					modified = true;
				}

				ImGui::SameLine();
				if (ImGui::Button("X")) {
					value->erase(value->begin() + i);
					modified = true;
					ImGui::PopID();
					break;
				}

				ImGui::PopID();
			}

			if (ImGui::Button("Add Item")) {
				value->push_back("");
				modified = true;
			}

			return modified;
		}

		static bool RenderArray(const std::string& label, std::vector<std::string>* value, const nlohmann::json& options = {}) {
			return RenderList(label, value, options); // For now, same as list
		}

		static bool Render(const std::string& label, std::vector<std::string>* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "list") {
				return RenderList(label, value, schema);
			}
			else if (widgetType == "array") {
				return RenderArray(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for string array property, defaulting to list" << std::endl;
				return RenderList(label, value, schema);
			}
		}
	};

} // namespace UISchema