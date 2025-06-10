/*
 * UISchemaUtils.hpp - Utility functions for UISchema
 * Handles schema parsing, widget type determination, and style management
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>

namespace UISchema {

	// Helper to get a value from schema with default
	template<typename T>
	static T GetSchemaValue(const nlohmann::json& schema, const std::string& key, const T& defaultValue) {
		if (schema.contains(key) && !schema[key].is_null()) {
			try {
				return schema[key].get<T>();
			}
			catch (const std::exception& e) {
				std::cerr << "Error parsing schema value for key " << key << ": " << e.what() << std::endl;
			}
		}
		return defaultValue;
	}

	// Default widget mappings
	namespace DefaultWidgets {
		static const std::unordered_map<std::string, std::string> TypeToWidget = {
			{"boolean", "checkbox"},
			{"integer", "input_int"},
			{"number", "input_float"},
			{"string", "input_text"},
			{"array", "combo"},
			{"object", "object"}
		};
	}

	// Widget type determination
	static std::string GetWidgetType(const nlohmann::json& schema) {
		// First check if widget type is explicitly specified
		if (schema.contains("ui:widget") && schema["ui:widget"].is_string()) {
			return schema["ui:widget"].get<std::string>();
		}

		// Otherwise infer from data type using default mappings
		if (schema.contains("type") && schema["type"].is_string()) {
			std::string type = schema["type"].get<std::string>();
			auto it = DefaultWidgets::TypeToWidget.find(type);
			if (it != DefaultWidgets::TypeToWidget.end()) {
				return it->second;
			}
		}

		// Default to simple text input
		return "input_text";
	}

	// ImGui flag extraction functions
	static ImGuiInputTextFlags GetInputTextFlags(const nlohmann::json& schema) {
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiInputTextFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiInputTextFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common text flags from schema options
		if (GetSchemaValue<bool>(schema, "readOnly", false)) {
			flags |= ImGuiInputTextFlags_ReadOnly;
		}

		if (GetSchemaValue<bool>(schema, "password", false)) {
			flags |= ImGuiInputTextFlags_Password;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.allowTabInput", false)) {
			flags |= ImGuiInputTextFlags_AllowTabInput;
		}

		return flags;
	}

	static ImGuiColorEditFlags GetColorEditFlags(const nlohmann::json& schema) {
		ImGuiColorEditFlags flags = ImGuiColorEditFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiColorEditFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiColorEditFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common color edit flags from schema options
		if (GetSchemaValue<bool>(schema, "ui:options.noAlpha", false)) {
			flags |= ImGuiColorEditFlags_NoAlpha;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.noPicker", false)) {
			flags |= ImGuiColorEditFlags_NoPicker;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.noOptions", false)) {
			flags |= ImGuiColorEditFlags_NoOptions;
		}

		std::string displayMode = GetSchemaValue<std::string>(schema, "ui:options.displayMode", "");
		if (displayMode == "rgb") {
			flags |= ImGuiColorEditFlags_DisplayRGB;
		}
		else if (displayMode == "hsv") {
			flags |= ImGuiColorEditFlags_DisplayHSV;
		}
		else if (displayMode == "hex") {
			flags |= ImGuiColorEditFlags_DisplayHex;
		}

		return flags;
	}

	static ImGuiSliderFlags GetSliderFlags(const nlohmann::json& schema) {
		ImGuiSliderFlags flags = ImGuiSliderFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiSliderFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiSliderFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common slider flags from schema options
		if (GetSchemaValue<bool>(schema, "ui:options.logarithmic", false)) {
			flags |= ImGuiSliderFlags_Logarithmic;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.noRoundToFormat", false)) {
			flags |= ImGuiSliderFlags_NoRoundToFormat;
		}

		return flags;
	}

	static ImGuiTableFlags GetTableFlags(const nlohmann::json& schema) {
		// Start with some sensible defaults
		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp;

		// Check for explicitly set flags
		if (schema.contains("flags")) {
			if (schema["flags"].is_number()) {
				return static_cast<ImGuiTableFlags>(schema["flags"].get<int>());
			}
			else if (schema["flags"].is_array()) {
				flags = ImGuiTableFlags_None;
				for (const auto& flag : schema["flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiTableFlags>(flag.get<int>());
					}
				}
			}
		}

		// Add flags based on options
		if (GetSchemaValue<bool>(schema, "resizable", true)) {
			flags |= ImGuiTableFlags_Resizable;
		}

		if (GetSchemaValue<bool>(schema, "scrollX", false)) {
			flags |= ImGuiTableFlags_ScrollX;
		}

		if (GetSchemaValue<bool>(schema, "scrollY", false)) {
			flags |= ImGuiTableFlags_ScrollY;
		}

		if (GetSchemaValue<bool>(schema, "hoverable", true)) {
			flags |= ImGuiTableFlags_Sortable;
		}

		return flags;
	}

	static ImGuiTabBarFlags GetTabBarFlags(const nlohmann::json& schema) {
		ImGuiTabBarFlags flags = ImGuiTabBarFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiTabBarFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiTabBarFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common tab bar flags from schema options
		if (GetSchemaValue<bool>(schema, "ui:options.reorderable", false)) {
			flags |= ImGuiTabBarFlags_Reorderable;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.noCloseWithMiddleMouseButton", false)) {
			flags |= ImGuiTabBarFlags_NoCloseWithMiddleMouseButton;
		}

		return flags;
	}

	static ImGuiTreeNodeFlags GetTreeNodeFlags(const nlohmann::json& schema) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiTreeNodeFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiTreeNodeFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common tree node flags from schema options
		if (GetSchemaValue<bool>(schema, "ui:options.defaultOpen", false)) {
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.openOnArrow", false)) {
			flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.openOnDoubleClick", false)) {
			flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		}

		return flags;
	}

	static ImGuiSelectableFlags GetSelectableFlags(const nlohmann::json& schema) {
		ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;

		// Check ui:flags directly
		if (schema.contains("ui:flags")) {
			if (schema["ui:flags"].is_number()) {
				return static_cast<ImGuiSelectableFlags>(schema["ui:flags"].get<int>());
			}
			else if (schema["ui:flags"].is_array()) {
				for (const auto& flag : schema["ui:flags"]) {
					if (flag.is_number()) {
						flags |= static_cast<ImGuiSelectableFlags>(flag.get<int>());
					}
				}
			}
		}

		// Common selectable flags from schema options
		if (GetSchemaValue<bool>(schema, "ui:options.allowDoubleClick", false)) {
			flags |= ImGuiSelectableFlags_AllowDoubleClick;
		}

		if (GetSchemaValue<bool>(schema, "ui:options.disabled", false)) {
			flags |= ImGuiSelectableFlags_Disabled;
		}

		return flags;
	}

	// Style management functions
	static void PushStyleFromSchema(const nlohmann::json& schema) {
		// Return early if no style defined
		if (!schema.contains("ui:style") || !schema["ui:style"].is_object()) {
			return;
		}

		const auto& style = schema["ui:style"];

		// ImGui text style
		if (style.contains("text")) {
			const auto& text = style["text"];
			if (text.contains("color") && text["color"].is_array() && text["color"].size() >= 3) {
				ImVec4 color(
					text["color"][0].get<float>(),
					text["color"][1].get<float>(),
					text["color"][2].get<float>(),
					text["color"].size() >= 4 ? text["color"][3].get<float>() : 1.0f
				);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			}
		}

		// Frame style
		if (style.contains("frame")) {
			const auto& frame = style["frame"];
			if (frame.contains("padding") && frame["padding"].is_array() && frame["padding"].size() >= 2) {
				ImVec2 padding(frame["padding"][0].get<float>(), frame["padding"][1].get<float>());
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
			}

			if (frame.contains("rounding") && frame["rounding"].is_number()) {
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, frame["rounding"].get<float>());
			}

			if (frame.contains("borderSize") && frame["borderSize"].is_number()) {
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, frame["borderSize"].get<float>());
			}

			if (frame.contains("bg") && frame["bg"].is_array() && frame["bg"].size() >= 3) {
				ImVec4 bg(
					frame["bg"][0].get<float>(),
					frame["bg"][1].get<float>(),
					frame["bg"][2].get<float>(),
					frame["bg"].size() >= 4 ? frame["bg"][3].get<float>() : 1.0f
				);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, bg);
			}
		}

		// Item style
		if (style.contains("item")) {
			const auto& item = style["item"];
			if (item.contains("spacing") && item["spacing"].is_array() && item["spacing"].size() >= 2) {
				ImVec2 spacing(item["spacing"][0].get<float>(), item["spacing"][1].get<float>());
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);
			}
		}
	}

	static void PopStyleFromSchema(const nlohmann::json& schema) {
		// Return early if no style defined
		if (!schema.contains("ui:style") || !schema["ui:style"].is_object()) {
			return;
		}

		const auto& style = schema["ui:style"];
		int colorPops = 0;
		int varPops = 0;

		// Count required color pops
		if (style.contains("text") && style["text"].contains("color")) colorPops++;
		if (style.contains("frame") && style["frame"].contains("bg")) colorPops++;

		// Count required var pops
		if (style.contains("frame")) {
			const auto& frame = style["frame"];
			if (frame.contains("padding")) varPops++;
			if (frame.contains("rounding")) varPops++;
			if (frame.contains("borderSize")) varPops++;
		}

		if (style.contains("item") && style["item"].contains("spacing")) varPops++;

		// Pop style vars and colors
		if (varPops > 0) ImGui::PopStyleVar(varPops);
		if (colorPops > 0) ImGui::PopStyleColor(colorPops);
	}

	// Helper functions for schema parsing
	static std::vector<std::string> GetPropertyOrder(const nlohmann::json& schema) {
		std::vector<std::string> propertyOrder;

		if (schema.contains("propertyOrder") && schema["propertyOrder"].is_array()) {
			for (const auto& item : schema["propertyOrder"]) {
				if (item.is_string()) {
					propertyOrder.push_back(item.get<std::string>());
				}
			}
		}
		else if (schema.contains("properties") && schema["properties"].is_object()) {
			// If no explicit order, use order in properties object
			for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
				propertyOrder.push_back(it.key());
			}
		}

		return propertyOrder;
	}

	static std::string GetPropertyLabel(const std::string& propName, const nlohmann::json& propSchema) {
		if (propSchema.contains("title") && propSchema["title"].is_string()) {
			return propSchema["title"].get<std::string>();
		}
		return propName;
	}

} // namespace UISchema