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
 * For commercial license iformation, please contact legal@kframe.ai.
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <unordered_map>
#include <variant>

namespace UISchema {

    // Type definitions for different property types
    using PropertyVariant = std::variant<
        bool*,                  // Boolean values (checkboxes)
        int*,                   // Integer values
        float*,                 // Float values
        double*,                // Double values
        std::string*,           // String values (text input)
        ImVec2*,                // 2D vector values
        ImVec4*,                // 4D vector/color values
        std::vector<std::string>*  // String arrays
    >;

    // Base function to render a schema
    bool RenderSchema(const nlohmann::json& schema,
        std::unordered_map<std::string, PropertyVariant>& properties);

    // Render an ImGui table based on a schema
    bool RenderTable(const nlohmann::json& schema,
        std::unordered_map<std::string, PropertyVariant>& properties);

    // Render different types of input widgets
    bool RenderCheckbox(const std::string& label, bool* value, const nlohmann::json& options = {});
    bool RenderInputInt(const std::string& label, int* value, const nlohmann::json& options = {});
    bool RenderInputFloat(const std::string& label, float* value, const nlohmann::json& options = {});
    bool RenderInputDouble(const std::string& label, double* value, const nlohmann::json& options = {});
    bool RenderInputText(const std::string& label, std::string* value, const nlohmann::json& options = {});
    bool RenderTextArea(const std::string& label, std::string* value, const nlohmann::json& options = {});
    bool RenderColorEdit(const std::string& label, ImVec4* value, const nlohmann::json& options = {});
    bool RenderVec2(const std::string& label, ImVec2* value, const nlohmann::json& options = {});
    bool RenderCombo(const std::string& label, int* selectedIndex, const std::vector<std::string>* items, const nlohmann::json& options = {});
    bool RenderSliderInt(const std::string& label, int* value, int min, int max, const nlohmann::json& options = {});
    bool RenderSliderFloat(const std::string& label, float* value, float min, float max, const nlohmann::json& options = {});
    bool RenderDragInt(const std::string& label, int* value, const nlohmann::json& options = {});
    bool RenderDragFloat(const std::string& label, float* value, const nlohmann::json& options = {});
    bool RenderRadioButtons(const std::string& label, int* value, const std::vector<std::string>* items, const nlohmann::json& options = {});

    // Helper functions for parsing schema options
    ImGuiInputTextFlags GetInputTextFlags(const nlohmann::json& schema);
    ImGuiColorEditFlags GetColorEditFlags(const nlohmann::json& schema);
    ImGuiSliderFlags GetSliderFlags(const nlohmann::json& schema);
    ImGuiTableFlags GetTableFlags(const nlohmann::json& schema);
    ImGuiTabBarFlags GetTabBarFlags(const nlohmann::json& schema);
    ImGuiTreeNodeFlags GetTreeNodeFlags(const nlohmann::json& schema);
    ImGuiSelectableFlags GetSelectableFlags(const nlohmann::json& schema);

    // Apply schema-defined style elements
    void PushStyleFromSchema(const nlohmann::json& schema);
    void PopStyleFromSchema(const nlohmann::json& schema);

    // Example usage for a prompt component:
    /*
    // Define your schema (usually loaded from a file)
    nlohmann::json promptSchema = {
        {"title", "Prompt Settings"},
        {"type", "object"},
        {"propertyOrder", {"posPrompt", "negPrompt"}},
        {"ui:table", {
            {"columns", 2},
            {"flags", ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp},
            {"columnSetup", {
                {"Param", ImGuiTableColumnFlags_WidthFixed, 52.0f},
                {"Value", ImGuiTableColumnFlags_WidthStretch}
            }}
        }},
        {"properties", {
            {"posPrompt", {
                {"type", "string"},
                {"title", "Positive"},
                {"ui:widget", "textarea"},
                {"ui:flags", {ImGuiInputTextFlags_AllowTabInput}},
                {"ui:options", {
                    {"rows", 8}
                }}
            }},
            {"negPrompt", {
                {"type", "string"},
                {"title", "Negative"},
                {"ui:widget", "textarea"},
                {"ui:flags", {ImGuiInputTextFlags_AllowTabInput}},
                {"ui:options", {
                    {"rows", 8}
                }}
            }}
        }}
    };

    // Map your component variables to property pointers
    std::unordered_map<std::string, UISchema::PropertyVariant> promptProps = {
        {"posPrompt", &promptComponent.posPrompt},
        {"negPrompt", &promptComponent.negPrompt}
    };

    // Render the UI from the schema
    bool modified = UISchema::RenderSchema(promptSchema, promptProps);
    */

} // namespace UISchema