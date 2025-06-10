/*
 * StringWidgets.hpp - Handles std::string* values from PropertyVariant
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include "UISchemaUtils.hpp"

namespace UISchema {

	static const std::string DEFAULT_STRING_WIDGET = "input_text";

	class StringWidgets {
	public:
		static bool RenderInputText(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);

			size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 1024);
			if (bufferSize < 1) bufferSize = 1;

			std::vector<char> buffer(bufferSize);
			strncpy(buffer.data(), value->c_str(), bufferSize - 1);
			buffer[bufferSize - 1] = '\0';

			bool changed = ImGui::InputText(label.c_str(), buffer.data(), bufferSize, flags);

			if (changed) {
				*value = buffer.data();
			}

			return changed;
		}

		static bool RenderTextArea(const std::string& label, std::string* value, const nlohmann::json& options = {}) {
			ImGuiInputTextFlags flags = GetInputTextFlags(options);

			int rows = GetSchemaValue<int>(options, "rows", 5);
			if (options.contains("ui:options") && options["ui:options"].is_object()) {
				const auto& uiOptions = options["ui:options"];
				if (uiOptions.contains("rows") && uiOptions["rows"].is_number()) {
					rows = uiOptions["rows"].get<int>();
				}
			}

			float lineHeight = ImGui::GetTextLineHeight();
			float height = lineHeight * rows + ImGui::GetStyle().FramePadding.y * 2.0f;

			size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 4096);
			if (bufferSize < 1) bufferSize = 1;

			std::vector<char> buffer(bufferSize);
			strncpy(buffer.data(), value->c_str(), bufferSize - 1);
			buffer[bufferSize - 1] = '\0';

			bool changed = ImGui::InputTextMultiline(
				label.c_str(),
				buffer.data(),
				bufferSize,
				ImVec2(-FLT_MIN, height),
				flags
			);

			if (changed) {
				*value = buffer.data();
			}

			return changed;
		}

		static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema) {
			if (widgetType == "input_text") {
				return RenderInputText(label, value, schema);
			}
			else if (widgetType == "textarea") {
				return RenderTextArea(label, value, schema);
			}
			else {
				std::cerr << "Unknown widget type '" << widgetType << "' for string property, defaulting to input_text" << std::endl;
				return RenderInputText(label, value, schema);
			}
		}
	};

} // namespace UISchema