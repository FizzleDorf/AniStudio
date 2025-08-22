#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "PropertyTypes.hpp"
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_INT_WIDGET = "input_int";

	class IntWidgets {
	public:
		static bool RenderInputInt(const std::string& label, int* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			int step = GetSchemaValue<int>(options, "step", 1);
			int step_fast = GetSchemaValue<int>(options, "step_fast", 100);
			int min = GetSchemaValue<int>(options, "min", INT_MIN);
			int max = GetSchemaValue<int>(options, "max", INT_MAX);

			// Store the original value
			int originalValue = *value;

			// Use ImGui::InputInt normally with the label
			bool changed = ImGui::InputInt(label.c_str(), value, step, step_fast, flags);

			// Apply constraints
			if (*value < min) *value = min;
			if (*value > max) *value = max;

			// Only return true if the value actually changed
			return changed && (*value != originalValue);
		}

		static bool RenderSliderInt(const std::string& label, int* value, int min, int max, const nlohmann::json& options = {}) {
			ImGuiSliderFlags flags = GetSliderFlags(options);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");

			// Store the original value to detect actual changes
			int originalValue = *value;
			bool changed = ImGui::SliderInt(label.c_str(), value, min, max, format.c_str(), flags);

			// Only return true if the value actually changed
			return changed && (*value != originalValue);
		}

		static bool RenderDragInt(const std::string& label, int* value, const nlohmann::json& options = {}) {
			float speed = GetSchemaValue<float>(options, "speed", 1.0f);
			int min = GetSchemaValue<int>(options, "minimum", 0);
			int max = GetSchemaValue<int>(options, "maximum", 0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");

			// Store the original value to detect actual changes
			int originalValue = *value;
			bool changed = ImGui::DragInt(label.c_str(), value, speed, min, max, format.c_str());

			// Only return true if the value actually changed
			return changed && (*value != originalValue);
		}

		static bool RenderCombo(const std::string& label, int* selectedIndex, const std::vector<std::string>* items, const nlohmann::json& options = {}) {
			if (!items || items->empty()) return false;

			// ImGui::Combo requires a const char* const*
			std::vector<const char*> c_items;
			for (const auto& item : *items) {
				c_items.push_back(item.c_str());
			}

			int originalValue = *selectedIndex;
			// The new ImGui::Combo overload takes a const char* const* for the items
			bool changed = ImGui::Combo(label.c_str(), selectedIndex, c_items.data(), c_items.size());

			// Only return true if the value actually changed
			return changed && (*selectedIndex != originalValue);
		}

		static bool RenderRadioButtons(const std::string& label, int* value, const std::vector<std::string>* items, const nlohmann::json& options = {}) {
			if (!items || items->empty()) return false;

			int originalValue = *value;
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

			// Only return true if the value actually changed
			return changed && (*value != originalValue);
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