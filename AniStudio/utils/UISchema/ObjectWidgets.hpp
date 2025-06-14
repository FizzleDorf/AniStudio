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
#include <variant>
#include <unordered_map>
#include "SchemaUtils.hpp"

namespace UISchema::ObjectWidgets {

	// Forward declaration of PropertyVariant
	using PropertyVariant = std::variant<
		bool*, int*, float*, double*, std::string*,
		ImVec2*, ImVec4*,
		std::vector<std::string>*, std::vector<int>*, std::vector<float>*
	>;

	// Forward declaration - this will be defined in the main UISchema.hpp
	namespace UISchema {
		bool RenderProperty(const std::string& label, const nlohmann::json& schema, PropertyVariant& property);
	}

	// Main render function for object widgets
	inline bool RenderObject(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		// Check for special layout types
		if (schema.contains("ui:layout")) {
			std::string layout = schema["ui:layout"].get<std::string>();

			if (layout == "table") {
				return RenderTable(schema, properties);
			}
			else if (layout == "tabs") {
				return RenderTabs(schema, properties);
			}
			else if (layout == "accordion") {
				return RenderAccordion(schema, properties);
			}
			else if (layout == "grid") {
				return RenderGrid(schema, properties);
			}
			else if (layout == "columns") {
				return RenderColumns(schema, properties);
			}
		}

		// Check for specific UI directives
		if (schema.contains("ui:table") && schema["ui:table"].is_object()) {
			return RenderTable(schema, properties);
		}
		else if (schema.contains("ui:tabs") && schema["ui:tabs"].is_object()) {
			return RenderTabs(schema, properties);
		}
		else if (schema.contains("ui:accordion") && schema["ui:accordion"].is_object()) {
			return RenderAccordion(schema, properties);
		}
		else {
			// Default object rendering
			return RenderDefault(schema, properties);
		}
	}

	// Render nested object property
	inline bool RenderProperty(const std::string& label, PropertyVariant& property, const nlohmann::json& schema) {
		// For now, nested objects are not implemented
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Nested objects not yet implemented: %s", label.c_str());
		return false;
	}

	// Default object rendering (simple vertical layout)
	inline bool RenderDefault(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		if (!schema.contains("properties") || !schema["properties"].is_object()) {
			return false;
		}

		bool modified = false;

		// Get property order
		std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

		// Render each property
		for (const auto& propName : propertyOrder) {
			if (schema["properties"].contains(propName)) {
				const auto& propSchema = schema["properties"][propName];

				// Check if property exists in our map
				if (properties.find(propName) == properties.end()) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Property not found: %s", propName.c_str());
					continue;
				}

				// Get label/title
				std::string label = SchemaUtils::GetPropertyLabel(propSchema, propName);

				// Render the property using the UISchema namespace function
				if (::UISchema::RenderProperty(label, propSchema, properties[propName])) {
					modified = true;
				}
			}
		}

		return modified;
	}

	// Table layout rendering
	inline bool RenderTable(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		if (!schema.contains("ui:table") || !schema["ui:table"].is_object()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid table schema");
			return false;
		}

		const auto& tableSchema = schema["ui:table"];

		// Get table settings
		int columns = SchemaUtils::GetSchemaValue<int>(tableSchema, "columns", 2);
		ImGuiTableFlags flags = SchemaUtils::GetTableFlags(tableSchema);

		// Start table
		bool tableCreated = ImGui::BeginTable("##SchemaTable", columns, flags);
		if (!tableCreated) return false;

		// Apply column setup if available
		if (tableSchema.contains("columnSetup") && tableSchema["columnSetup"].is_object()) {
			for (auto it = tableSchema["columnSetup"].begin(); it != tableSchema["columnSetup"].end(); ++it) {
				const std::string& colName = it.key();

				// Different ways to specify column flags/width
				if (it.value().is_array() && it.value().size() >= 2) {
					ImGuiTableColumnFlags colFlags = ImGuiTableColumnFlags_None;
					float width = 0.0f;

					if (it.value()[0].is_number()) {
						colFlags = static_cast<ImGuiTableColumnFlags>(it.value()[0].get<int>());
					}

					if (it.value()[1].is_number()) {
						width = it.value()[1].get<float>();
					}

					ImGui::TableSetupColumn(colName.c_str(), colFlags, width);
				}
				else {
					// Default setup if not specified properly
					ImGui::TableSetupColumn(colName.c_str());
				}
			}
		}
		else {
			// Default column setup
			for (int i = 0; i < columns; i++) {
				ImGui::TableSetupColumn(("Col" + std::to_string(i)).c_str());
			}
		}

		// Show headers if requested
		bool showHeaders = SchemaUtils::GetSchemaValue<bool>(tableSchema, "showHeaders", false);
		if (showHeaders) {
			ImGui::TableHeadersRow();
		}

		bool modified = false;

		// Get property order
		std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

		// Render each property in a table row
		if (schema.contains("properties") && schema["properties"].is_object()) {
			for (const auto& propName : propertyOrder) {
				if (schema["properties"].contains(propName)) {
					const auto& propSchema = schema["properties"][propName];

					// Skip if property not found in our map
					if (properties.find(propName) == properties.end()) continue;

					ImGui::TableNextRow();

					// Get label/title
					std::string label = SchemaUtils::GetPropertyLabel(propSchema, propName);

					// First column for label
					ImGui::TableNextColumn();
					ImGui::Text("%s", label.c_str());

					// Second column for widget
					ImGui::TableNextColumn();

					// Push ID to avoid conflicts when same property appears multiple times
					ImGui::PushID(propName.c_str());

					// Render the widget without the label (since label is in first column)
					PropertyVariant& propVariant = properties[propName];
					if (RenderTableWidget(propSchema, propVariant)) {
						modified = true;
					}

					ImGui::PopID();
				}
			}
		}

		ImGui::EndTable();
		return modified;
	}

	// Render widget for table context (without labels)
	inline bool RenderTableWidget(const nlohmann::json& schema, PropertyVariant& property) {
		std::string dataType = SchemaUtils::GetSchemaValue<std::string>(schema, "type", "string");
		std::string widgetType = SchemaUtils::GetWidgetType(schema);

		try {
			// Boolean widgets
			if (dataType == "boolean" && std::holds_alternative<bool*>(property)) {
				bool* value = std::get<bool*>(property);
				return ImGui::Checkbox("##value", value);
			}

			// Integer widgets
			else if (dataType == "integer" && std::holds_alternative<int*>(property)) {
				int* value = std::get<int*>(property);

				if (widgetType == "slider_int") {
					int min = SchemaUtils::GetSchemaValue<int>(schema, "minimum", 0);
					int max = SchemaUtils::GetSchemaValue<int>(schema, "maximum", 100);
					ImGuiSliderFlags flags = SchemaUtils::GetSliderFlags(schema);
					return ImGui::SliderInt("##value", value, min, max, "%d", flags);
				}
				else if (widgetType == "drag_int") {
					float speed = SchemaUtils::GetSchemaValue<float>(schema, "speed", 1.0f);
					int min = SchemaUtils::GetSchemaValue<int>(schema, "minimum", 0);
					int max = SchemaUtils::GetSchemaValue<int>(schema, "maximum", 0);
					return ImGui::DragInt("##value", value, speed, min, max);
				}
				else {
					ImGuiInputTextFlags flags = SchemaUtils::GetInputTextFlags(schema);
					return ImGui::InputInt("##value", value, 1, 100, flags);
				}
			}

			// Float widgets
			else if (dataType == "number" && std::holds_alternative<float*>(property)) {
				float* value = std::get<float*>(property);

				if (widgetType == "slider_float") {
					float min = SchemaUtils::GetSchemaValue<float>(schema, "minimum", 0.0f);
					float max = SchemaUtils::GetSchemaValue<float>(schema, "maximum", 1.0f);
					ImGuiSliderFlags flags = SchemaUtils::GetSliderFlags(schema);
					return ImGui::SliderFloat("##value", value, min, max, "%.3f", flags);
				}
				else if (widgetType == "drag_float") {
					float speed = SchemaUtils::GetSchemaValue<float>(schema, "speed", 0.1f);
					float min = SchemaUtils::GetSchemaValue<float>(schema, "minimum", 0.0f);
					float max = SchemaUtils::GetSchemaValue<float>(schema, "maximum", 0.0f);
					return ImGui::DragFloat("##value", value, speed, min, max);
				}
				else {
					ImGuiInputTextFlags flags = SchemaUtils::GetInputTextFlags(schema);
					return ImGui::InputFloat("##value", value, 0.05f, 1.0f, "%.3f", flags);
				}
			}

			// Double widgets
			else if (dataType == "number" && std::holds_alternative<double*>(property)) {
				double* value = std::get<double*>(property);
				ImGuiInputTextFlags flags = SchemaUtils::GetInputTextFlags(schema);
				return ImGui::InputDouble("##value", value, 0.0, 0.0, "%.6f", flags);
			}

			// String widgets
			else if (dataType == "string" && std::holds_alternative<std::string*>(property)) {
				std::string* value = std::get<std::string*>(property);

				if (widgetType == "textarea") {
					ImGuiInputTextFlags flags = SchemaUtils::GetInputTextFlags(schema);
					int rows = SchemaUtils::GetSchemaValue<int>(schema, "ui:options.rows", 5);
					float height = ImGui::GetTextLineHeight() * rows + ImGui::GetStyle().FramePadding.y * 2.0f;

					std::vector<char> buffer(8193);
					strncpy(buffer.data(), value->c_str(), 8192);
					buffer[8192] = '\0';

					if (ImGui::InputTextMultiline("##value", buffer.data(), 8193,
						ImVec2(-FLT_MIN, height), flags)) {
						*value = buffer.data();
						return true;
					}
				}
				else {
					ImGuiInputTextFlags flags = SchemaUtils::GetInputTextFlags(schema);
					std::vector<char> buffer(1024);
					strncpy(buffer.data(), value->c_str(), buffer.size() - 1);
					buffer[buffer.size() - 1] = '\0';

					if (ImGui::InputText("##value", buffer.data(), buffer.size(), flags)) {
						*value = buffer.data();
						return true;
					}
				}
			}

			// Vector widgets
			else if (widgetType == "color" && std::holds_alternative<ImVec4*>(property)) {
				ImVec4* value = std::get<ImVec4*>(property);
				ImGuiColorEditFlags flags = SchemaUtils::GetColorEditFlags(schema);
				return ImGui::ColorEdit4("##value", &value->x, flags);
			}
			else if (widgetType == "vec2" && std::holds_alternative<ImVec2*>(property)) {
				ImVec2* value = std::get<ImVec2*>(property);
				return ImGui::InputFloat2("##value", &value->x);
			}
			else if (widgetType == "vec4" && std::holds_alternative<ImVec4*>(property)) {
				ImVec4* value = std::get<ImVec4*>(property);
				return ImGui::InputFloat4("##value", &value->x);
			}

			else {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Unsupported widget or type mismatch");
			}
		}
		catch (const std::exception& e) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
		}

		return false;
	}

	// Tabs layout rendering
	inline bool RenderTabs(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		if (!schema.contains("ui:tabs") || !schema["ui:tabs"].is_object()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid tabs schema");
			return false;
		}

		const auto& tabsSchema = schema["ui:tabs"];
		bool modified = false;

		// Get tab bar flags
		ImGuiTabBarFlags flags = ImGuiTabBarFlags_None;
		if (tabsSchema.contains("flags")) {
			flags = static_cast<ImGuiTabBarFlags>(
				SchemaUtils::GetSchemaValue<int>(tabsSchema, "flags", 0));
		}

		if (ImGui::BeginTabBar("##SchemaTabBar", flags)) {

			// Get property order for tabs
			std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

			// Render each property as a tab
			if (schema.contains("properties") && schema["properties"].is_object()) {
				for (const auto& propName : propertyOrder) {
					if (schema["properties"].contains(propName)) {
						const auto& propSchema = schema["properties"][propName];
						std::string tabLabel = SchemaUtils::GetPropertyLabel(propSchema, propName);

						if (ImGui::BeginTabItem(tabLabel.c_str())) {

							// Skip if property not found in our map
							if (properties.find(propName) != properties.end()) {
								PropertyVariant& propVariant = properties[propName];
								if (::UISchema::RenderProperty(tabLabel, propSchema, propVariant)) {
									modified = true;
								}
							}
							else {
								ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
									"Property not found: %s", propName.c_str());
							}

							ImGui::EndTabItem();
						}
					}
				}
			}

			ImGui::EndTabBar();
		}

		return modified;
	}

	// Accordion/Collapsible layout rendering
	inline bool RenderAccordion(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		if (!schema.contains("ui:accordion") || !schema["ui:accordion"].is_object()) {
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid accordion schema");
			return false;
		}

		const auto& accordionSchema = schema["ui:accordion"];
		bool modified = false;

		// Default open state
		bool defaultOpen = SchemaUtils::GetSchemaValue<bool>(accordionSchema, "defaultOpen", false);

		// Get property order for accordion sections
		std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

		// Render each property as a collapsible section
		if (schema.contains("properties") && schema["properties"].is_object()) {
			for (const auto& propName : propertyOrder) {
				if (schema["properties"].contains(propName)) {
					const auto& propSchema = schema["properties"][propName];
					std::string sectionLabel = SchemaUtils::GetPropertyLabel(propSchema, propName);

					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader;
					if (defaultOpen) {
						flags |= ImGuiTreeNodeFlags_DefaultOpen;
					}

					if (ImGui::CollapsingHeader(sectionLabel.c_str(), flags)) {

						// Indent content
						ImGui::Indent();

						// Skip if property not found in our map
						if (properties.find(propName) != properties.end()) {
							PropertyVariant& propVariant = properties[propName];
							if (::UISchema::RenderProperty(sectionLabel, propSchema, propVariant)) {
								modified = true;
							}
						}
						else {
							ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
								"Property not found: %s", propName.c_str());
						}

						ImGui::Unindent();
					}
				}
			}
		}

		return modified;
	}

	// Grid layout rendering
	inline bool RenderGrid(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		int columns = SchemaUtils::GetSchemaValue<int>(schema, "ui:grid.columns", 2);

		bool modified = false;
		std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

		if (schema.contains("properties") && schema["properties"].is_object()) {
			int currentColumn = 0;

			for (const auto& propName : propertyOrder) {
				if (schema["properties"].contains(propName)) {
					const auto& propSchema = schema["properties"][propName];

					// Skip if property not found in our map
					if (properties.find(propName) == properties.end()) continue;

					// Start new row if needed
					if (currentColumn > 0) {
						ImGui::SameLine();
					}

					// Set column width
					float columnWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * (columns - 1)) / columns;
					ImGui::BeginGroup();
					ImGui::PushItemWidth(columnWidth);

					// Get label/title
					std::string label = SchemaUtils::GetPropertyLabel(propSchema, propName);

					// Render the property
					PropertyVariant& propVariant = properties[propName];
					if (::UISchema::RenderProperty(label, propSchema, propVariant)) {
						modified = true;
					}

					ImGui::PopItemWidth();
					ImGui::EndGroup();

					currentColumn = (currentColumn + 1) % columns;
				}
			}
		}

		return modified;
	}

	// Columns layout rendering
	inline bool RenderColumns(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
		if (!ImGui::BeginTable("##ColumnsLayout", 2, ImGuiTableFlags_SizingStretchProp)) {
			return false;
		}

		ImGui::TableSetupColumn("Column1");
		ImGui::TableSetupColumn("Column2");

		bool modified = false;
		std::vector<std::string> propertyOrder = SchemaUtils::GetPropertyOrder(schema);

		if (schema.contains("properties") && schema["properties"].is_object()) {
			size_t propertyIndex = 0;

			for (const auto& propName : propertyOrder) {
				if (schema["properties"].contains(propName)) {
					const auto& propSchema = schema["properties"][propName];

					// Skip if property not found in our map
					if (properties.find(propName) == properties.end()) continue;

					// Start new row for every two properties
					if (propertyIndex % 2 == 0) {
						ImGui::TableNextRow();
					}

					ImGui::TableNextColumn();

					// Get label/title
					std::string label = SchemaUtils::GetPropertyLabel(propSchema, propName);

					// Render the property
					PropertyVariant& propVariant = properties[propName];
					if (::UISchema::RenderProperty(label, propSchema, propVariant)) {
						modified = true;
					}

					propertyIndex++;
				}
			}
		}

		ImGui::EndTable();
		return modified;
	}

} // namespace UISchema::ObjectWidgets