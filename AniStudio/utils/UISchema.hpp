// UISchema.hpp
#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstring>

namespace UISchema {

    using json = nlohmann::json;

    // Path of the last modified widget
    inline std::string lastModifiedPath;

    // Helper function to get default value from schema
    json getDefaultValue(const json& schema) {
        if (schema.contains("default")) {
            return schema["default"];
        }

        if (schema["type"] == "string") return "";
        if (schema["type"] == "float") return 0.0f;
        if (schema["type"] == "int") return 0;
        if (schema["type"] == "bool") return false;
        if (schema["type"] == "array") return json::array();
        if (schema["type"] == "object") return json::object();

        return json();
    }

    // Main function to draw a component UI with optional buffer support
    bool drawComponent(const char* label, json& values, const json& schema, json& cache,
        std::map<std::string, std::pair<char*, size_t>>* buffers = nullptr) {
        bool modified = false;
        lastModifiedPath = "";

        // Only process objects with properties
        if (schema["type"] != "object" || !schema.contains("properties")) {
            return false;
        }

        // If cache is not an object, initialize it
        if (!cache.is_object()) {
            cache = json::object();
        }

        // Begin a table for layout
        if (ImGui::BeginTable("UISchema_Table", 2, ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            // Get properties in the order they were defined
            std::vector<std::string> propOrder;

            // Use explicit property order if available
            if (schema.contains("propertyOrder")) {
                for (const auto& prop : schema["propertyOrder"]) {
                    propOrder.push_back(prop.get<std::string>());
                }
            }
            else {
                // Fall back to iteration order from the schema
                for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
                    propOrder.push_back(it.key());
                }
            }

            // Process each property in the schema in the determined order
            for (const auto& propName : propOrder) {
                // Skip if property doesn't exist in schema
                if (!schema["properties"].contains(propName)) {
                    continue;
                }

                const auto& propSchema = schema["properties"][propName];

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // Display property title/label
                std::string propTitle = propSchema.value("title", propName);
                ImGui::Text("%s", propTitle.c_str());

                // Show tooltip if there's a description
                if (propSchema.contains("description") && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", propSchema["description"].get<std::string>().c_str());
                }

                ImGui::TableNextColumn();

                // Generate a unique ID for this control
                std::string id = "##" + propName;
                std::string path = std::string(label) + "/" + propName;

                // Make sure value exists
                if (!values.contains(propName)) {
                    values[propName] = getDefaultValue(propSchema);
                }

                // Handle different types of widgets
                std::string type = propSchema.value("type", "");
                std::string widgetType = propSchema.value("ui:widget", "");

                ImGui::PushItemWidth(-FLT_MIN);

                if (type == "string") {
                    // Check if we have a textarea with external buffer
                    if (widgetType == "textarea" && buffers && buffers->count(path) > 0) {
                        // Get rows
                        int rows = 4;
                        if (propSchema.contains("ui:options") && propSchema["ui:options"].contains("rows")) {
                            rows = propSchema["ui:options"]["rows"];
                        }

                        // Get the buffer and its size
                        auto& [buffer, size] = (*buffers)[path];

                        // Draw the textarea using the external buffer
                        ImVec2 textareaSize(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * rows);
                        if (ImGui::InputTextMultiline(id.c_str(), buffer, size, textareaSize, ImGuiInputTextFlags_AllowTabInput)) {
                            values[propName] = std::string(buffer);
                            modified = true;
                            lastModifiedPath = path;
                        }
                    }
                    else if (widgetType == "textarea") {
                        // Regular textarea without external buffer
                        int rows = 4;
                        if (propSchema.contains("ui:options") && propSchema["ui:options"].contains("rows")) {
                            rows = propSchema["ui:options"]["rows"];
                        }

                        std::string value = values[propName].get<std::string>();
                        char buffer[4096]; // Using a reasonably sized static buffer
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';

                        ImVec2 size(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * rows);
                        if (ImGui::InputTextMultiline(id.c_str(), buffer, sizeof(buffer), size, ImGuiInputTextFlags_AllowTabInput)) {
                            values[propName] = std::string(buffer);
                            modified = true;
                            lastModifiedPath = path;
                        }
                    }
                    else {
                        // Simple text input
                        std::string value = values[propName].get<std::string>();
                        char buffer[1024];
                        strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';

                        if (ImGui::InputText(id.c_str(), buffer, sizeof(buffer))) {
                            values[propName] = std::string(buffer);
                            modified = true;
                            lastModifiedPath = path;
                        }
                    }
                }
                else if (type == "int") {
                    int val = values[propName].get<int>();
                    if (ImGui::InputInt(id.c_str(), &val)) {
                        values[propName] = val;
                        modified = true;
                        lastModifiedPath = path;
                    }
                }
                else if (type == "float") {
                    float val = values[propName].get<float>();
                    if (ImGui::InputFloat(id.c_str(), &val)) {
                        values[propName] = val;
                        modified = true;
                        lastModifiedPath = path;
                    }
                }
                else if (type == "bool") {
                    bool val = values[propName].get<bool>();
                    if (ImGui::Checkbox(id.c_str(), &val)) {
                        values[propName] = val;
                        modified = true;
                        lastModifiedPath = path;
                    }
                }

                ImGui::PopItemWidth();
            }

            ImGui::EndTable();
        }

        return modified;
    }

    // Convenience function for direct buffer support
    inline bool drawComponentWithBuffers(const char* label, json& values, const json& schema, json& cache,
        std::map<std::string, std::pair<char*, size_t>>& buffers) {
        return drawComponent(label, values, schema, cache, &buffers);
    }

    // Get the path of the last modified property
    const std::string& getLastModifiedPath() {
        return lastModifiedPath;
    }

} // namespace UISchema