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
#include <unordered_map>
#include <variant>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	using PropertyVariant = std::variant<bool*, int*, float*, double*, std::string*, ImVec2*, ImVec4*, std::vector<std::string>*>;
	using PropertyMap = std::unordered_map<std::string, PropertyVariant>;

	static const std::string DEFAULT_INT_WIDGET = "input_int";

	class IntWidgets {
	public:
		static bool RenderInputInt(const std::string& label, int* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			int step = GetSchemaValue<int>(options, "step", 1);
			int step_fast = GetSchemaValue<int>(options, "step_fast", 100);
			return ImGui::InputInt(label.c_str(), value, step, step_fast, flags);
		}

		static bool RenderSliderInt(const std::string& label, int* value, int min, int max, const nlohmann::json& options = {}) {
			ImGuiSliderFlags flags = GetSliderFlags(options);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			return ImGui::SliderInt(label.c_str(), value, min, max, format.c_str(), flags);
		}

		static bool RenderDragInt(const std::string& label, int* value, const nlohmann::json& options = {}) {
			float speed = GetSchemaValue<float>(options, "speed", 1.0f);
			int min = GetSchemaValue<int>(options, "minimum", 0);
			int max = GetSchemaValue<int>(options, "maximum", 0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			return ImGui::DragInt(label.c_str(), value, speed, min, max, format.c_str());
		}

		static bool RenderCombo(const std::string& label, int* selectedIndex, const std::vector<std::string>* items, const nlohmann::json& options = {}) {
			if (!items || items->empty()) return false;

			auto itemGetter = [](void* data, int idx, const char** out_text) -> bool {
				auto* items = static_cast<const std::vector<std::string>*>(data);
				if (idx < 0 || idx >= static_cast<int>(items->size())) return false;
				*out_text = (*items)[idx].c_str();
				return true;
			};

			return ImGui::Combo(label.c_str(), selectedIndex, itemGetter,
				const_cast<void*>(static_cast<const void*>(items)),
				static_cast<int>(items->size()));
		}

		static bool RenderRadioButtons(const std::string& label, int* value, const std::vector<std::string>* items, const nlohmann::json& options = {}) {
			if (!items || items->empty()) return false;

			bool changed = false;
			if (!label.empty()) {
				ImGui::Text("%s", label.c_str());
			}

			for (int i = 0; i < static_cast<int>(items->size()); i++) {
				if (ImGui::RadioButton((*items)[i].c_str(), value, i)) {
					changed = true;
				}

				bool horizontal = GetSchemaValue<bool>(options, "horizontal", false);
				if (horizontal && i < static_cast<int>(items->size()) - 1) {
					ImGui::SameLine();
				}
			}

			return changed;
		}

		static bool Render(const std::string& label, int* value, const std::string& widgetType, const nlohmann::json& schema, const PropertyMap& allProps = {}) {
			if (widgetType == "input_int") {
				return RenderInputInt(label, value, schema);
			}
			else if (widgetType == "slider_int") {
				int min = GetSchemaValue<int>(schema, "minimum", 0);
				int max = GetSchemaValue<int>(schema, "maximum", 100);
				return RenderSliderInt(label, value, min, max, schema);
			}
			else if (widgetType == "drag_int") {
				return RenderDragInt(label, value, schema);
			}
			else if (widgetType == "combo") {
				if (schema.contains("items") && schema["items"].is_array()) {
					std::vector<std::string> items;
					for (const auto& item : schema["items"]) {
						if (item.is_string()) {
							items.push_back(item.get<std::string>());
						}
						else if (item.is_object() && item.contains("label") && item["label"].is_string()) {
							items.push_back(item["label"].get<std::string>());
						}
					}
					return RenderCombo(label, value, &items, schema);
				}
				else if (allProps.count("items") && std::holds_alternative<std::vector<std::string>*>(allProps.at("items"))) {
					std::vector<std::string>* items = std::get<std::vector<std::string>*>(allProps.at("items"));
					return RenderCombo(label, value, items, schema);
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Combo widget missing items array");
					return false;
				}
			}
			else if (widgetType == "radio") {
				if (allProps.count("items") && std::holds_alternative<std::vector<std::string>*>(allProps.at("items"))) {
					std::vector<std::string>* items = std::get<std::vector<std::string>*>(allProps.at("items"));
					return RenderRadioButtons(label, value, items, schema);
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Radio widget missing items array");
					return false;
				}
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for int property, defaulting to input_int" << std::endl;
				return RenderInputInt(label, value, schema);
			}
		}
	};

} // namespace UISchema