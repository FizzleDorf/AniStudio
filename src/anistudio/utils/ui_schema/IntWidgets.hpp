#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "PropertyTypes.hpp"
#include "UISchemaUtils.hpp"
#include "UISchemaContext.hpp"

namespace UISchema {

	static const std::string DEFAULT_INT_WIDGET = "input_int";

	class IntWidgets {
	public:
		static bool RenderInputInt(const std::string& label, int* value, const nlohmann::json& options, const UIRenderContext& context) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			int step = GetSchemaValue<int>(options, "step", 1);
			int step_fast = GetSchemaValue<int>(options, "step_fast", 100);
			int min = GetSchemaValue<int>(options, "min", INT_MIN);
			int max = GetSchemaValue<int>(options, "max", INT_MAX);

			int originalValue = *value;
			bool changed = ImGui::InputInt(label.c_str(), value, step, step_fast, flags);
			if (*value < min) *value = min;
			if (*value > max) *value = max;
			return changed && (*value != originalValue);
		}

		static bool RenderSliderInt(const std::string& label, int* value, int min, int max, const nlohmann::json& options, const UIRenderContext& context) {
			ImGuiSliderFlags flags = GetSliderFlags(options);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			int originalValue = *value;
			bool changed = ImGui::SliderInt(label.c_str(), value, min, max, format.c_str(), flags);
			return changed && (*value != originalValue);
		}

		static bool RenderDragInt(const std::string& label, int* value, const nlohmann::json& options, const UIRenderContext& context) {
			float speed = GetSchemaValue<float>(options, "speed", 1.0f);
			int min = GetSchemaValue<int>(options, "min", 0);
			int max = GetSchemaValue<int>(options, "max", 0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			int originalValue = *value;
			bool changed = ImGui::DragInt(label.c_str(), value, speed, min, max, format.c_str());
			return changed && (*value != originalValue);
		}

		static bool RenderCombo(const std::string& label, int* selectedIndex, const std::vector<std::string>* items, const nlohmann::json& options, const UIRenderContext& context) {
			if (!items || items->empty()) return false;
			std::vector<const char*> c_items;
			for (const auto& item : *items) c_items.push_back(item.c_str());
			int originalValue = *selectedIndex;
			bool changed = ImGui::Combo(label.c_str(), selectedIndex, c_items.data(), c_items.size());
			return changed && (*selectedIndex != originalValue);
		}

		static bool RenderRadioButtons(const std::string& label, int* value, const std::vector<std::string>* items, const nlohmann::json& options, const UIRenderContext& context) {
			if (!items || items->empty()) return false;
			int originalValue = *value;
			bool changed = false;
			std::string propertyName = "radio";
			size_t hashPos = label.find("##");
			if (hashPos != std::string::npos) {
				std::string uniquePart = label.substr(hashPos + 2);
				size_t underscorePos = uniquePart.find('_');
				if (underscorePos != std::string::npos) propertyName = uniquePart.substr(0, underscorePos);
			}
			if (!label.empty() && label.find("##") != std::string::npos) {
				std::string displayName = label.substr(0, label.find("##"));
				if (!displayName.empty()) ImGui::Text("%s", displayName.c_str());
			}
			for (int i = 0; i < static_cast<int>(items->size()); i++) {
				std::string radioId = context.GenerateWidgetId(propertyName, "radio_" + std::to_string(i));
				std::string radioLabel = (*items)[i] + "##" + radioId;
				if (ImGui::RadioButton(radioLabel.c_str(), value, i)) changed = true;
				bool horizontal = GetSchemaValue<bool>(options, "horizontal", false);
				if (horizontal && i < static_cast<int>(items->size()) - 1) ImGui::SameLine();
			}
			return changed && (*value != originalValue);
		}

		static bool RenderInputInt64(const std::string& label, int64_t* value, const nlohmann::json& options, const UIRenderContext& context) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);
			int64_t step = GetSchemaValue<int64_t>(options, "step", 1);
			int64_t step_fast = GetSchemaValue<int64_t>(options, "step_fast", 100);
			int64_t min = GetSchemaValue<int64_t>(options, "min", INT64_MIN);
			int64_t max = GetSchemaValue<int64_t>(options, "max", INT64_MAX);
			int64_t originalValue = *value;
			bool changed = ImGui::InputScalar(label.c_str(), ImGuiDataType_S64, value, &step, &step_fast, NULL, flags);
			if (*value < min) *value = min;
			if (*value > max) *value = max;
			return changed && (*value != originalValue);
		}

		static bool RenderSliderInt64(const std::string& label, int64_t* value, int64_t min, int64_t max, const nlohmann::json& options, const UIRenderContext& context) {
			ImGuiSliderFlags flags = GetSliderFlags(options);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			int64_t originalValue = *value;
			bool changed = ImGui::SliderScalar(label.c_str(), ImGuiDataType_S64, value, &min, &max, format.c_str(), flags);
			return changed && (*value != originalValue);
		}

		static bool RenderDragInt64(const std::string& label, int64_t* value, const nlohmann::json& options, const UIRenderContext& context) {
			float speed = GetSchemaValue<float>(options, "speed", 1.0f);
			int64_t min = GetSchemaValue<int64_t>(options, "min", 0);
			int64_t max = GetSchemaValue<int64_t>(options, "max", 0);
			std::string format = GetSchemaValue<std::string>(options, "format", "%d");
			int64_t originalValue = *value;
			bool changed = ImGui::DragScalar(label.c_str(), ImGuiDataType_S64, value, speed, &min, &max, format.c_str());
			return changed && (*value != originalValue);
		}

		static bool Render(const std::string& label, int* value, const std::string& widgetType, const nlohmann::json& schema, const PropertyMap& allProps, const UIRenderContext& context) {
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) options = schema["ui:options"];
			if (widgetType == "input_int") return RenderInputInt(label, value, options, context);
			else if (widgetType == "slider_int") {
				int min = GetSchemaValue<int>(options, "min", 0);
				int max = GetSchemaValue<int>(options, "max", 100);
				if (min == 0 && max == 100) {
					min = GetSchemaValue<int>(schema, "minimum", 0);
					max = GetSchemaValue<int>(schema, "maximum", 100);
				}
				return RenderSliderInt(label, value, min, max, options, context);
			}
			else if (widgetType == "drag_int") return RenderDragInt(label, value, options, context);
			else if (widgetType == "combo") {
				if (schema.contains("items") && schema["items"].is_array()) {
					std::vector<std::string> items;
					for (const auto& item : schema["items"]) {
						if (item.is_string()) items.push_back(item.get<std::string>());
						else if (item.is_object() && item.contains("label") && item["label"].is_string()) items.push_back(item["label"].get<std::string>());
					}
					return RenderCombo(label, value, &items, options, context);
				}
				else if (allProps.count("items") && std::holds_alternative<std::vector<std::string>*>(allProps.at("items"))) {
					std::vector<std::string>* items = std::get<std::vector<std::string>*>(allProps.at("items"));
					return RenderCombo(label, value, items, options, context);
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Combo widget missing items array");
					return false;
				}
			}
			else if (widgetType == "radio") {
				if (allProps.count("items") && std::holds_alternative<std::vector<std::string>*>(allProps.at("items"))) {
					std::vector<std::string>* items = std::get<std::vector<std::string>*>(allProps.at("items"));
					return RenderRadioButtons(label, value, items, options, context);
				}
				else {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Radio widget missing items array");
					return false;
				}
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for int property, defaulting to input_int" << std::endl;
				return RenderInputInt(label, value, options, context);
			}
		}

		static bool Render(const std::string& label, int64_t* value, const std::string& widgetType, const nlohmann::json& schema, const PropertyMap& allProps, const UIRenderContext& context) {
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) options = schema["ui:options"];
			if (widgetType == "input_int") return RenderInputInt64(label, value, options, context);
			else if (widgetType == "slider_int") {
				int64_t min = GetSchemaValue<int64_t>(options, "min", 0);
				int64_t max = GetSchemaValue<int64_t>(options, "max", 100);
				if (min == 0 && max == 100) {
					min = GetSchemaValue<int64_t>(schema, "minimum", 0);
					max = GetSchemaValue<int64_t>(schema, "maximum", 100);
				}
				return RenderSliderInt64(label, value, min, max, options, context);
			}
			else if (widgetType == "drag_int") return RenderDragInt64(label, value, options, context);
			else if (widgetType == "combo" || widgetType == "radio") {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Combo/radio not supported for int64_t");
				return false;
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for int64 property, defaulting to input_int" << std::endl;
				return RenderInputInt64(label, value, options, context);
			}
		}
	};

} // namespace UISchema