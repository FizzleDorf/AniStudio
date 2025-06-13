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
#include <unordered_map>
#include <variant>
#include <vector>
#include <iostream>

#include "UISchema/UISchemaUtils.hpp"
#include "UISchema/BoolWidgets.hpp"
#include "UISchema/IntWidgets.hpp"
#include "UISchema/FloatWidgets.hpp"
#include "UISchema/DoubleWidgets.hpp"
#include "UISchema/StringWidgets.hpp"
#include "UISchema/Vec2Widgets.hpp"
#include "UISchema/Vec4Widgets.hpp"
#include "UISchema/StringArrayWidgets.hpp"

namespace UISchema {

	using PropertyVariant = std::variant<bool*, int*, float*, double*, std::string*, ImVec2*, ImVec4*, std::vector<std::string>*>;
	using PropertyMap = std::unordered_map<std::string, PropertyVariant>;

	// Window state management for separate windows
	static std::unordered_map<std::string, bool>& GetWindowStates() {
		static std::unordered_map<std::string, bool> windowStates;
		return windowStates;
	}

	// Forward declarations
	static bool RenderPropertiesForm(const nlohmann::json& schema, PropertyMap& properties);
	static bool RenderPropertyWidget(const std::string& label, const std::string& widgetType,
		PropertyVariant& propVariant, const nlohmann::json& propSchema,
		const PropertyMap& allProperties = {});
	static bool RenderTable(const nlohmann::json& schema, PropertyMap& properties);
	static bool RenderTableWidget(const std::string& widgetType, PropertyVariant& propVariant,
		const nlohmann::json& propSchema, const PropertyMap& allProperties);
	static void SetupTableColumns(const nlohmann::json& tableSchema, int columns);
	static bool RenderSeparateWindows(const nlohmann::json& schema, PropertyMap& properties);

	// Main rendering functions
	static bool RenderSchema(const nlohmann::json& schema, PropertyMap& properties) {
		if (!schema.is_object()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid schema: not an object");
			return false;
		}

		PushStyleFromSchema(schema);
		bool modified = false;

		if (schema.contains("title") && schema["title"].is_string()) {
			ImGui::Text("%s", schema["title"].get<std::string>().c_str());
			ImGui::Separator();
		}

		// NEW: Check if this schema should create separate windows
		if (schema.contains("ui:separate_windows") && schema["ui:separate_windows"].get<bool>()) {
			modified = RenderSeparateWindows(schema, properties);
		}
		else if (schema.contains("ui:table") && schema["ui:table"].is_object()) {
			modified = RenderTable(schema, properties);
		}
		else if (schema.contains("properties") && schema["properties"].is_object()) {
			modified = RenderPropertiesForm(schema, properties);
		}

		PopStyleFromSchema(schema);
		return modified;
	}

	// NEW: Handle separate window rendering - COMPLETELY SELF-CONTAINED
	// NEW: Handle separate window rendering - COMPLETELY SELF-CONTAINED
	static bool RenderSeparateWindows(const nlohmann::json& schema, PropertyMap& properties) {
		bool modified = false;
		auto& windowStates = GetWindowStates();

		if (!schema.contains("properties") || !schema["properties"].is_object()) {
			return false;
		}

		std::vector<std::string> propertyOrder = GetPropertyOrder(schema);

		for (const auto& propName : propertyOrder) {
			if (schema["properties"].contains(propName)) {
				const auto& propSchema = schema["properties"][propName];

				// Check if this property should have its own window
				if (propSchema.contains("ui:window") && propSchema["ui:window"].get<bool>()) {

					if (properties.find(propName) == properties.end()) {
						ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Property not found: %s", propName.c_str());
						continue;
					}

					// Get window name
					std::string windowName = propName;
					if (propSchema.contains("ui:window_name") && propSchema["ui:window_name"].is_string()) {
						windowName = propSchema["ui:window_name"].get<std::string>();
					}

					// Create unique window ID
					uintptr_t propAddress = 0;
					if (std::holds_alternative<std::string*>(properties[propName])) {
						propAddress = reinterpret_cast<uintptr_t>(std::get<std::string*>(properties[propName]));
					}
					std::string windowId = windowName + "##" + std::to_string(propAddress);

					// Initialize window state
					if (windowStates.find(windowId) == windowStates.end()) {
						windowStates[windowId] = false;
					}

					// Render checkbox to toggle window
					std::string checkboxLabel = windowName + " Editor";
					ImGui::Checkbox(checkboxLabel.c_str(), &windowStates[windowId]);

					// Show status
					ImGui::SameLine();
					if (std::holds_alternative<std::string*>(properties[propName])) {
						std::string* strPtr = std::get<std::string*>(properties[propName]);
						ImGui::Text("(%zu chars)", strPtr->length());
					}

					// RENDER THE WINDOW IMMEDIATELY IF IT SHOULD BE SHOWN
					if (windowStates[windowId]) {
						bool& showWindow = windowStates[windowId];

						if (ImGui::Begin(windowId.c_str(), &showWindow)) {

							std::string widgetType = GetWidgetType(propSchema);
							PropertyVariant& propVariant = properties[propName];

							try {
								if (RenderPropertyWidget("##" + propName, widgetType, propVariant, propSchema, properties)) {
									modified = true;
								}
							}
							catch (const std::exception& e) {
								ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
									"Error rendering property %s: %s", propName.c_str(), e.what());
							}
						}
						ImGui::End();
					}
				}
				else {
					// Regular property rendering for properties without ui:window
					std::string widgetType = GetWidgetType(propSchema);
					std::string label = GetPropertyLabel(propName, propSchema);
					PropertyVariant& propVariant = properties[propName];

					try {
						if (RenderPropertyWidget(label, widgetType, propVariant, propSchema, properties)) {
							modified = true;
						}
					}
					catch (const std::exception& e) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
							"Error rendering property %s: %s", propName.c_str(), e.what());
					}
				}
			}
		}

		return modified;
	}

	static bool RenderPropertiesForm(const nlohmann::json& schema, PropertyMap& properties) {
		bool modified = false;
		std::vector<std::string> propertyOrder = GetPropertyOrder(schema);

		for (const auto& propName : propertyOrder) {
			if (schema["properties"].contains(propName)) {
				const auto& propSchema = schema["properties"][propName];

				if (properties.find(propName) == properties.end()) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Property not found: %s", propName.c_str());
					continue;
				}

				std::string widgetType = GetWidgetType(propSchema);
				std::string label = GetPropertyLabel(propName, propSchema);
				PropertyVariant& propVariant = properties[propName];

				try {
					if (RenderPropertyWidget(label, widgetType, propVariant, propSchema, properties)) {
						modified = true;
					}
				}
				catch (const std::exception& e) {
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
						"Error rendering property %s: %s", propName.c_str(), e.what());
				}
			}
		}

		return modified;
	}

	static bool RenderPropertyWidget(const std::string& label, const std::string& widgetType,
		PropertyVariant& propVariant, const nlohmann::json& propSchema,
		const PropertyMap& allProperties) {

		if (std::holds_alternative<bool*>(propVariant)) {
			bool* value = std::get<bool*>(propVariant);
			return BoolWidgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<int*>(propVariant)) {
			int* value = std::get<int*>(propVariant);
			return IntWidgets::Render(label, value, widgetType, propSchema, allProperties);
		}
		else if (std::holds_alternative<float*>(propVariant)) {
			float* value = std::get<float*>(propVariant);
			return FloatWidgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<double*>(propVariant)) {
			double* value = std::get<double*>(propVariant);
			return DoubleWidgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<std::string*>(propVariant)) {
			std::string* value = std::get<std::string*>(propVariant);
			return StringWidgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<ImVec2*>(propVariant)) {
			ImVec2* value = std::get<ImVec2*>(propVariant);
			return Vec2Widgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<ImVec4*>(propVariant)) {
			ImVec4* value = std::get<ImVec4*>(propVariant);
			return Vec4Widgets::Render(label, value, widgetType, propSchema);
		}
		else if (std::holds_alternative<std::vector<std::string>*>(propVariant)) {
			std::vector<std::string>* value = std::get<std::vector<std::string>*>(propVariant);
			return StringArrayWidgets::Render(label, value, widgetType, propSchema);
		}

		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Unsupported property type");
		return false;
	}

	static bool RenderTable(const nlohmann::json& schema, PropertyMap& properties) {
		if (!schema.contains("ui:table") || !schema["ui:table"].is_object()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid table schema");
			return false;
		}

		const auto& tableSchema = schema["ui:table"];
		int columns = GetSchemaValue<int>(tableSchema, "columns", 2);
		ImGuiTableFlags flags = GetTableFlags(tableSchema);

		bool tableCreated = ImGui::BeginTable("##SchemaTable", columns, flags);
		if (!tableCreated) return false;

		SetupTableColumns(tableSchema, columns);

		bool showHeaders = GetSchemaValue<bool>(tableSchema, "showHeaders", false);
		if (showHeaders) {
			ImGui::TableHeadersRow();
		}

		bool modified = false;
		std::vector<std::string> propertyOrder = GetPropertyOrder(schema);

		if (schema.contains("properties") && schema["properties"].is_object()) {
			for (const auto& propName : propertyOrder) {
				if (schema["properties"].contains(propName)) {
					const auto& propSchema = schema["properties"][propName];

					if (properties.find(propName) == properties.end()) continue;

					ImGui::TableNextRow();
					std::string label = GetPropertyLabel(propName, propSchema);

					ImGui::TableNextColumn();
					ImGui::Text("%s", label.c_str());

					ImGui::TableNextColumn();
					std::string widgetType = GetWidgetType(propSchema);
					PropertyVariant& propVariant = properties[propName];

					ImGui::PushID(propName.c_str());

					try {
						if (RenderTableWidget(widgetType, propVariant, propSchema, properties)) {
							modified = true;
						}
					}
					catch (const std::exception& e) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
					}

					ImGui::PopID();
				}
			}
		}

		ImGui::EndTable();
		return modified;
	}

	static bool RenderTableWidget(const std::string& widgetType, PropertyVariant& propVariant,
		const nlohmann::json& propSchema, const PropertyMap& allProperties) {
		// Use empty label for table widgets since label is in first column
		return RenderPropertyWidget("##value", widgetType, propVariant, propSchema, allProperties);
	}

	static void SetupTableColumns(const nlohmann::json& tableSchema, int columns) {
		if (tableSchema.contains("columnSetup") && tableSchema["columnSetup"].is_object()) {
			for (auto it = tableSchema["columnSetup"].begin(); it != tableSchema["columnSetup"].end(); ++it) {
				const std::string& colName = it.key();

				if (it.value().is_array() && it.value().size() >= 2) {
					ImGuiTableColumnFlags colFlags = static_cast<ImGuiTableColumnFlags>(it.value()[0].get<int>());
					float width = it.value()[1].get<float>();
					ImGui::TableSetupColumn(colName.c_str(), colFlags, width);
				}
				else {
					ImGui::TableSetupColumn(colName.c_str());
				}
			}
		}
		else {
			for (int i = 0; i < columns; i++) {
				ImGui::TableSetupColumn(("Col" + std::to_string(i)).c_str());
			}
		}
	}

	/*
	// Example usage for a prompt component with separate windows:
	// The UISchema handles EVERYTHING automatically - no additional calls needed!

	nlohmann::json promptSchema = {
		{"title", "Prompt Settings"},
		{"type", "object"},
		{"propertyOrder", {"posPrompt", "negPrompt"}},
		{"ui:separate_windows", true},  // Enable separate window mode
		{"properties", {
			{"posPrompt", {
				{"type", "string"},
				{"title", "Positive"},
				{"ui:widget", "zep_editor_toolbar"},
				{"ui:window", true},  // Create separate window
				{"ui:window_name", "Positive"},  // Window name
				{"ui:options", {
					{"width", 800.0f},
					{"height", 400.0f}
				}}
			}},
			{"negPrompt", {
				{"type", "string"},
				{"title", "Negative"},
				{"ui:widget", "zep_editor_toolbar"},
				{"ui:window", true},  // Create separate window
				{"ui:window_name", "Negative"},  // Window name
				{"ui:options", {
					{"width", 800.0f},
					{"height", 300.0f}
				}}
			}}
		}}
	};

	// Usage in your view - THAT'S IT! UISchema handles everything:
	auto promptProps = promptComponent.GetPropertyMap();
	UISchema::RenderSchema(promptSchema, promptProps);
	// ^ This shows buttons AND automatically renders "Positive" and "Negative" windows!
	*/

} // namespace UISchema