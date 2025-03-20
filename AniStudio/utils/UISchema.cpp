#include "UISchema.hpp"
#include <imgui.h>
#include <string>
#include <variant>
#include <iostream>

namespace UISchema {

    // Helper to get a value from schema with default
    template<typename T>
    T GetSchemaValue(const nlohmann::json& schema, const std::string& key, const T& defaultValue) {
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

    // Helper function to determine widget type from schema
    std::string GetWidgetType(const nlohmann::json& schema) {
        // First check if widget type is explicitly specified
        if (schema.contains("ui:widget") && schema["ui:widget"].is_string()) {
            return schema["ui:widget"].get<std::string>();
        }

        // Otherwise infer from data type
        if (schema.contains("type") && schema["type"].is_string()) {
            std::string type = schema["type"].get<std::string>();
            if (type == "boolean") return "checkbox";
            if (type == "integer") return "input_int";
            if (type == "number") return "input_float";
            if (type == "string") return "input_text";
            if (type == "array") return "combo";
            if (type == "object") return "object";
        }

        // Default to simple text input
        return "input_text";
    }

    // Main schema rendering function
    bool RenderSchema(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
        if (!schema.is_object()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid schema: not an object");
            return false;
        }

        // Apply any style settings from schema
        PushStyleFromSchema(schema);

        bool modified = false;

        // Display title if present
        if (schema.contains("title") && schema["title"].is_string()) {
            ImGui::Text("%s", schema["title"].get<std::string>().c_str());
            ImGui::Separator();
        }

        // Check if we should render as a table
        if (schema.contains("ui:table") && schema["ui:table"].is_object()) {
            modified = RenderTable(schema, properties);
        }
        else if (schema.contains("properties") && schema["properties"].is_object()) {
            // Get property order
            std::vector<std::string> propertyOrder;

            if (schema.contains("propertyOrder") && schema["propertyOrder"].is_array()) {
                for (const auto& item : schema["propertyOrder"]) {
                    if (item.is_string()) {
                        propertyOrder.push_back(item.get<std::string>());
                    }
                }
            }

            // If no explicit order, use order in properties object
            if (propertyOrder.empty()) {
                for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
                    propertyOrder.push_back(it.key());
                }
            }

            // Render each property
            for (const auto& propName : propertyOrder) {
                if (schema["properties"].contains(propName)) {
                    const auto& propSchema = schema["properties"][propName];

                    // Check if property exists in our map
                    if (properties.find(propName) == properties.end()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Property not found: %s", propName.c_str());
                        continue;
                    }

                    // Get widget type
                    std::string widgetType = GetWidgetType(propSchema);

                    // Get label/title
                    std::string label = propName;
                    if (propSchema.contains("title") && propSchema["title"].is_string()) {
                        label = propSchema["title"].get<std::string>();
                    }

                    // Render the appropriate widget based on type
                    PropertyVariant& propVariant = properties[propName];

                    try {
                        if (widgetType == "checkbox" && std::holds_alternative<bool*>(propVariant)) {
                            bool* value = std::get<bool*>(propVariant);
                            if (RenderCheckbox(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            if (RenderInputInt(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            if (RenderInputFloat(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_double" && std::holds_alternative<double*>(propVariant)) {
                            double* value = std::get<double*>(propVariant);
                            if (RenderInputDouble(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_text" && std::holds_alternative<std::string*>(propVariant)) {
                            std::string* value = std::get<std::string*>(propVariant);
                            if (RenderInputText(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "textarea" && std::holds_alternative<std::string*>(propVariant)) {
                            std::string* value = std::get<std::string*>(propVariant);
                            if (RenderTextArea(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "color" && std::holds_alternative<ImVec4*>(propVariant)) {
                            ImVec4* value = std::get<ImVec4*>(propVariant);
                            if (RenderColorEdit(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "vec2" && std::holds_alternative<ImVec2*>(propVariant)) {
                            ImVec2* value = std::get<ImVec2*>(propVariant);
                            if (RenderVec2(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "combo" && std::holds_alternative<int*>(propVariant) &&
                            propSchema.contains("items") && propSchema["items"].is_array()) {
                            // Create items list from schema
                            std::vector<std::string> items;
                            for (const auto& item : propSchema["items"]) {
                                if (item.is_string()) {
                                    items.push_back(item.get<std::string>());
                                }
                                else if (item.is_object() && item.contains("label") && item["label"].is_string()) {
                                    items.push_back(item["label"].get<std::string>());
                                }
                            }

                            int* value = std::get<int*>(propVariant);
                            if (RenderCombo(label, value, &items, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "combo" && std::holds_alternative<int*>(propVariant) &&
                            std::holds_alternative<std::vector<std::string>*>(properties["items"])) {
                            int* value = std::get<int*>(propVariant);
                            std::vector<std::string>* items = std::get<std::vector<std::string>*>(properties["items"]);
                            if (RenderCombo(label, value, items, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "slider_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            int min = GetSchemaValue<int>(propSchema, "minimum", 0);
                            int max = GetSchemaValue<int>(propSchema, "maximum", 100);
                            if (RenderSliderInt(label, value, min, max, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "slider_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            float min = GetSchemaValue<float>(propSchema, "minimum", 0.0f);
                            float max = GetSchemaValue<float>(propSchema, "maximum", 1.0f);
                            if (RenderSliderFloat(label, value, min, max, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "drag_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            if (RenderDragInt(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "drag_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            if (RenderDragFloat(label, value, propSchema)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "radio" && std::holds_alternative<int*>(propVariant) &&
                            std::holds_alternative<std::vector<std::string>*>(properties["items"])) {
                            int* value = std::get<int*>(propVariant);
                            std::vector<std::string>* items = std::get<std::vector<std::string>*>(properties["items"]);
                            if (RenderRadioButtons(label, value, items, propSchema)) {
                                modified = true;
                            }
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                                "Unsupported widget type or property type mismatch: %s (%s)",
                                propName.c_str(), widgetType.c_str());
                        }
                    }
                    catch (const std::exception& e) {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                            "Error rendering property %s: %s", propName.c_str(), e.what());
                    }
                }
            }
        }

        // Restore any style changes
        PopStyleFromSchema(schema);

        return modified;
    }

    // Render a table-based layout
    bool RenderTable(const nlohmann::json& schema, std::unordered_map<std::string, PropertyVariant>& properties) {
        if (!schema.contains("ui:table") || !schema["ui:table"].is_object()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid table schema");
            return false;
        }

        const auto& tableSchema = schema["ui:table"];

        // Get table settings
        int columns = GetSchemaValue<int>(tableSchema, "columns", 2);
        ImGuiTableFlags flags = GetTableFlags(tableSchema);

        // Start table
        bool tableCreated = ImGui::BeginTable("##SchemaTable", columns, flags);
        if (!tableCreated) return false;

        // Apply column setup if available
        if (tableSchema.contains("columnSetup") && tableSchema["columnSetup"].is_object()) {
            for (auto it = tableSchema["columnSetup"].begin(); it != tableSchema["columnSetup"].end(); ++it) {
                const std::string& colName = it.key();

                // Different ways to specify column flags/width
                if (it.value().is_array()) {
                    ImGuiTableColumnFlags colFlags = ImGuiTableColumnFlags_None;
                    float width = 0.0f;

                    if (it.value().size() >= 1 && it.value()[0].is_number()) {
                        colFlags = static_cast<ImGuiTableColumnFlags>(it.value()[0].get<int>());
                    }

                    if (it.value().size() >= 2 && it.value()[1].is_number()) {
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
        bool showHeaders = GetSchemaValue<bool>(tableSchema, "showHeaders", false);
        if (showHeaders) {
            ImGui::TableHeadersRow();
        }

        bool modified = false;

        // Get property order
        std::vector<std::string> propertyOrder;
        if (schema.contains("propertyOrder") && schema["propertyOrder"].is_array()) {
            for (const auto& item : schema["propertyOrder"]) {
                if (item.is_string()) {
                    propertyOrder.push_back(item.get<std::string>());
                }
            }
        }
        else if (schema.contains("properties") && schema["properties"].is_object()) {
            for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
                propertyOrder.push_back(it.key());
            }
        }

        // Render each property in a table row
        if (schema.contains("properties") && schema["properties"].is_object()) {
            for (const auto& propName : propertyOrder) {
                if (schema["properties"].contains(propName)) {
                    const auto& propSchema = schema["properties"][propName];

                    // Skip if property not found in our map
                    if (properties.find(propName) == properties.end()) continue;

                    ImGui::TableNextRow();

                    // Get label/title
                    std::string label = propName;
                    if (propSchema.contains("title") && propSchema["title"].is_string()) {
                        label = propSchema["title"].get<std::string>();
                    }

                    // First column for label
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", label.c_str());

                    // Second column for widget
                    ImGui::TableNextColumn();

                    // Get widget type
                    std::string widgetType = GetWidgetType(propSchema);
                    PropertyVariant& propVariant = properties[propName];

                    // Push ID to avoid conflicts when same property appears multiple times
                    ImGui::PushID(propName.c_str());

                    try {
                        // Render the appropriate widget based on the property type
                        // Similar to the non-table version but without labels
                        if (widgetType == "checkbox" && std::holds_alternative<bool*>(propVariant)) {
                            bool* value = std::get<bool*>(propVariant);
                            if (ImGui::Checkbox("##value", value)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            ImGuiInputTextFlags flags = GetInputTextFlags(propSchema);
                            if (ImGui::InputInt("##value", value, 1, 100, flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            ImGuiInputTextFlags flags = GetInputTextFlags(propSchema);
                            if (ImGui::InputFloat("##value", value, 0.0f, 0.0f, "%.3f", flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_double" && std::holds_alternative<double*>(propVariant)) {
                            double* value = std::get<double*>(propVariant);
                            ImGuiInputTextFlags flags = GetInputTextFlags(propSchema);
                            if (ImGui::InputDouble("##value", value, 0.0, 0.0, "%.6f", flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "input_text" && std::holds_alternative<std::string*>(propVariant)) {
                            std::string* value = std::get<std::string*>(propVariant);
                            ImGuiInputTextFlags flags = GetInputTextFlags(propSchema);

                            // Allocate buffer for text input
                            char buffer[1024] = { 0 };
                            strncpy(buffer, value->c_str(), sizeof(buffer) - 1);

                            if (ImGui::InputText("##value", buffer, sizeof(buffer), flags)) {
                                *value = buffer;
                                modified = true;
                            }
                        }
                        else if (widgetType == "textarea" && std::holds_alternative<std::string*>(propVariant)) {
                            std::string* value = std::get<std::string*>(propVariant);

                            // Get text area options
                            ImGuiInputTextFlags flags = GetInputTextFlags(propSchema);
                            int rows = 5;  // Default rows

                            if (propSchema.contains("ui:options") && propSchema["ui:options"].is_object()) {
                                const auto& options = propSchema["ui:options"];
                                if (options.contains("rows") && options["rows"].is_number()) {
                                    rows = options["rows"].get<int>();
                                }
                            }

                            // Calculate height based on rows
                            float height = ImGui::GetTextLineHeight() * rows + ImGui::GetStyle().FramePadding.y * 2.0f;

                            // Create text buffer
                            char* buffer = new char[8193];
                            strncpy(buffer, value->c_str(), 8192);
                            buffer[8192] = '\0';

                            if (ImGui::InputTextMultiline("##value", buffer, 8193,
                                ImVec2(-FLT_MIN, height), flags)) {
                                *value = buffer;
                                modified = true;
                            }

                            delete[] buffer;
                        }
                        else if (widgetType == "color" && std::holds_alternative<ImVec4*>(propVariant)) {
                            ImVec4* value = std::get<ImVec4*>(propVariant);
                            ImGuiColorEditFlags flags = GetColorEditFlags(propSchema);
                            if (ImGui::ColorEdit4("##value", &value->x, flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "vec2" && std::holds_alternative<ImVec2*>(propVariant)) {
                            ImVec2* value = std::get<ImVec2*>(propVariant);
                            if (ImGui::InputFloat2("##value", &value->x)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "combo" && std::holds_alternative<int*>(propVariant) &&
                            propSchema.contains("items") && propSchema["items"].is_array()) {
                            // Create items list from schema
                            std::vector<std::string> items;
                            for (const auto& item : propSchema["items"]) {
                                if (item.is_string()) {
                                    items.push_back(item.get<std::string>());
                                }
                                else if (item.is_object() && item.contains("label") && item["label"].is_string()) {
                                    items.push_back(item["label"].get<std::string>());
                                }
                            }

                            int* value = std::get<int*>(propVariant);
                            if (ImGui::Combo("##value", value,
                                [](void* data, int idx, const char** out_text) {
                                    auto* items = static_cast<std::vector<std::string>*>(data);
                                    if (idx < 0 || idx >= static_cast<int>(items->size())) return false;
                                    *out_text = (*items)[idx].c_str();
                                    return true;
                                },
                                &items, static_cast<int>(items.size()))) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "slider_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            int min = GetSchemaValue<int>(propSchema, "minimum", 0);
                            int max = GetSchemaValue<int>(propSchema, "maximum", 100);
                            ImGuiSliderFlags flags = GetSliderFlags(propSchema);
                            if (ImGui::SliderInt("##value", value, min, max, "%d", flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "slider_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            float min = GetSchemaValue<float>(propSchema, "minimum", 0.0f);
                            float max = GetSchemaValue<float>(propSchema, "maximum", 1.0f);
                            ImGuiSliderFlags flags = GetSliderFlags(propSchema);
                            if (ImGui::SliderFloat("##value", value, min, max, "%.3f", flags)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "drag_int" && std::holds_alternative<int*>(propVariant)) {
                            int* value = std::get<int*>(propVariant);
                            float speed = GetSchemaValue<float>(propSchema, "speed", 1.0f);
                            int min = GetSchemaValue<int>(propSchema, "minimum", 0);
                            int max = GetSchemaValue<int>(propSchema, "maximum", 0);
                            if (ImGui::DragInt("##value", value, speed, min, max)) {
                                modified = true;
                            }
                        }
                        else if (widgetType == "drag_float" && std::holds_alternative<float*>(propVariant)) {
                            float* value = std::get<float*>(propVariant);
                            float speed = GetSchemaValue<float>(propSchema, "speed", 0.1f);
                            float min = GetSchemaValue<float>(propSchema, "minimum", 0.0f);
                            float max = GetSchemaValue<float>(propSchema, "maximum", 0.0f);
                            if (ImGui::DragFloat("##value", value, speed, min, max)) {
                                modified = true;
                            }
                        }
                        else {
                            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Unsupported widget or type mismatch");
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

    // Individual widget renderers
    bool RenderCheckbox(const std::string& label, bool* value, const nlohmann::json& options) {
        return ImGui::Checkbox(label.c_str(), value);
    }

    bool RenderInputInt(const std::string& label, int* value, const nlohmann::json& options) {
        ImGuiInputTextFlags flags = GetInputTextFlags(options);
        int step = GetSchemaValue<int>(options, "step", 1);
        int step_fast = GetSchemaValue<int>(options, "step_fast", 100);
        return ImGui::InputInt(label.c_str(), value, step, step_fast, flags);
    }

    bool RenderInputFloat(const std::string& label, float* value, const nlohmann::json& options) {
        ImGuiInputTextFlags flags = GetInputTextFlags(options);
        float step = GetSchemaValue<float>(options, "step", 0.0f);
        float step_fast = GetSchemaValue<float>(options, "step_fast", 0.0f);
        std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
        return ImGui::InputFloat(label.c_str(), value, step, step_fast, format.c_str(), flags);
    }

    bool RenderInputDouble(const std::string& label, double* value, const nlohmann::json& options) {
        ImGuiInputTextFlags flags = GetInputTextFlags(options);
        double step = GetSchemaValue<double>(options, "step", 0.0);
        double step_fast = GetSchemaValue<double>(options, "step_fast", 0.0);
        std::string format = GetSchemaValue<std::string>(options, "format", "%.6f");
        return ImGui::InputDouble(label.c_str(), value, step, step_fast, format.c_str(), flags);
    }

    bool RenderInputText(const std::string& label, std::string* value, const nlohmann::json& options) {
        ImGuiInputTextFlags flags = GetInputTextFlags(options);

        // Calculate max buffer size - default or from schema
        size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 1024);
        if (bufferSize < 1) bufferSize = 1;  // Ensure at least 1 character

        // Create buffer and copy string
        std::vector<char> buffer(bufferSize);
        strncpy(buffer.data(), value->c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';  // Ensure null termination

        // Render the input text widget
        bool changed = ImGui::InputText(label.c_str(), buffer.data(), bufferSize, flags);

        // If changed, update the string
        if (changed) {
            *value = buffer.data();
        }

        return changed;
    }

    bool RenderTextArea(const std::string& label, std::string* value, const nlohmann::json& options) {
        ImGuiInputTextFlags flags = GetInputTextFlags(options);

        // Calculate height based on rows
        int rows = GetSchemaValue<int>(options, "ui:options.rows", 5);
        float lineHeight = ImGui::GetTextLineHeight();
        float height = lineHeight * rows + ImGui::GetStyle().FramePadding.y * 2.0f;

        // Calculate max buffer size - default or from schema
        size_t bufferSize = GetSchemaValue<size_t>(options, "maxLength", 4096);
        if (bufferSize < 1) bufferSize = 1;  // Ensure at least 1 character

        // Create buffer and copy string
        std::vector<char> buffer(bufferSize);
        strncpy(buffer.data(), value->c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';  // Ensure null termination

        // Render the multiline input widget
        bool changed = ImGui::InputTextMultiline(
            label.c_str(),
            buffer.data(),
            bufferSize,
            ImVec2(-FLT_MIN, height),  // Width uses available space
            flags
        );

        // If changed, update the string
        if (changed) {
            *value = buffer.data();
        }

        return changed;
    }

    bool RenderColorEdit(const std::string& label, ImVec4* value, const nlohmann::json& options) {
        ImGuiColorEditFlags flags = GetColorEditFlags(options);
        return ImGui::ColorEdit4(label.c_str(), &value->x, flags);
    }

    bool RenderVec2(const std::string& label, ImVec2* value, const nlohmann::json& options) {
        std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
        ImGuiInputTextFlags flags = GetInputTextFlags(options);
        return ImGui::InputFloat2(label.c_str(), &value->x, format.c_str(), flags);
    }

    bool RenderCombo(const std::string& label, int* selectedIndex, const std::vector<std::string>* items, const nlohmann::json& options) {
        if (!items || items->empty()) return false;

        // Custom item getter for std::vector<std::string>
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

    bool RenderSliderInt(const std::string& label, int* value, int min, int max, const nlohmann::json& options) {
        ImGuiSliderFlags flags = GetSliderFlags(options);
        std::string format = GetSchemaValue<std::string>(options, "format", "%d");
        return ImGui::SliderInt(label.c_str(), value, min, max, format.c_str(), flags);
    }

    bool RenderSliderFloat(const std::string& label, float* value, float min, float max, const nlohmann::json& options) {
        ImGuiSliderFlags flags = GetSliderFlags(options);
        std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
        return ImGui::SliderFloat(label.c_str(), value, min, max, format.c_str(), flags);
    }

    bool RenderDragInt(const std::string& label, int* value, const nlohmann::json& options) {
        float speed = GetSchemaValue<float>(options, "speed", 1.0f);
        int min = GetSchemaValue<int>(options, "minimum", 0);
        int max = GetSchemaValue<int>(options, "maximum", 0);
        std::string format = GetSchemaValue<std::string>(options, "format", "%d");
        return ImGui::DragInt(label.c_str(), value, speed, min, max, format.c_str());
    }

    bool RenderDragFloat(const std::string& label, float* value, const nlohmann::json& options) {
        float speed = GetSchemaValue<float>(options, "speed", 0.1f);
        float min = GetSchemaValue<float>(options, "minimum", 0.0f);
        float max = GetSchemaValue<float>(options, "maximum", 0.0f);
        std::string format = GetSchemaValue<std::string>(options, "format", "%.3f");
        return ImGui::DragFloat(label.c_str(), value, speed, min, max, format.c_str());
    }

    bool RenderRadioButtons(const std::string& label, int* value, const std::vector<std::string>* items, const nlohmann::json& options) {
        if (!items || items->empty()) return false;

        bool changed = false;
        if (!label.empty()) {
            ImGui::Text("%s", label.c_str());
        }

        for (int i = 0; i < static_cast<int>(items->size()); i++) {
            if (ImGui::RadioButton((*items)[i].c_str(), value, i)) {
                changed = true;
            }

            // Optional horizontal layout
            bool horizontal = GetSchemaValue<bool>(options, "horizontal", false);
            if (horizontal && i < static_cast<int>(items->size()) - 1) {
                ImGui::SameLine();
            }
        }

        return changed;
    }

    // Helper functions for parsing schema options
    ImGuiInputTextFlags GetInputTextFlags(const nlohmann::json& schema) {
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

    ImGuiColorEditFlags GetColorEditFlags(const nlohmann::json& schema) {
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

    ImGuiSliderFlags GetSliderFlags(const nlohmann::json& schema) {
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

    ImGuiTableFlags GetTableFlags(const nlohmann::json& schema) {
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

    ImGuiTabBarFlags GetTabBarFlags(const nlohmann::json& schema) {
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

    ImGuiTreeNodeFlags GetTreeNodeFlags(const nlohmann::json& schema) {
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

    ImGuiSelectableFlags GetSelectableFlags(const nlohmann::json& schema) {
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

    // Apply schema-defined style elements
    void PushStyleFromSchema(const nlohmann::json& schema) {
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

    void PopStyleFromSchema(const nlohmann::json& schema) {
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
        ImGui::PopStyleVar(varPops);
        ImGui::PopStyleColor(colorPops);
    }

} // namespace UISchema