/*
 * StringArrayWidgets.hpp - Handles std::vector<std::string>* values from PropertyVariant
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