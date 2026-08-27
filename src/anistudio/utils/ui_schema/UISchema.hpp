#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <iostream>
#include <functional>

#include "PropertyTypes.hpp"
#include "UISchemaUtils.hpp"
#include "UISchemaContext.hpp"
#include "BoolWidgets.hpp"
#include "IntWidgets.hpp"
#include "FloatWidgets.hpp"
#include "DoubleWidgets.hpp"
#include "StringWidgets.hpp"
#include "Vec2Widgets.hpp"
#include "Vec4Widgets.hpp"
#include "StringArrayWidgets.hpp"
#include "FileDialogWidgets.hpp"
#include "MediaLoadingWidgets.hpp"

namespace UISchema {

    static std::unordered_map<std::string, bool>& GetWindowStates() {
        static std::unordered_map<std::string, bool> windowStates;
        return windowStates;
    }

    static void RenderTooltipIfPresent(const nlohmann::json& propSchema, float delay = 0.5f) {
        std::string tooltipText;
        if (propSchema.contains("ui:tooltip") && propSchema["ui:tooltip"].is_string()) {
            tooltipText = propSchema["ui:tooltip"].get<std::string>();
        }
        else if (propSchema.contains("description") && propSchema["description"].is_string()) {
            tooltipText = propSchema["description"].get<std::string>();
        }
        if (!tooltipText.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(tooltipText.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    static nlohmann::json GetPropertyValueAsJson(const Engine::PropertyVariant& propVariant) {
        if (auto* p = std::get_if<bool*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<int*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<int64_t*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<void*>(&propVariant)) {
            int64_t* val = reinterpret_cast<int64_t*>(*p);
            return *val;
        }
        else if (auto* p = std::get_if<float*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<double*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<std::string*>(&propVariant)) {
            return **p;
        }
        else if (auto* p = std::get_if<ImVec2*>(&propVariant)) {
            return nlohmann::json::array({ (*p)->x, (*p)->y });
        }
        else if (auto* p = std::get_if<ImVec4*>(&propVariant)) {
            return nlohmann::json::array({ (*p)->x, (*p)->y, (*p)->z, (*p)->w });
        }
        else if (auto* p = std::get_if<std::vector<std::string>*>(&propVariant)) {
            return nlohmann::json(**p);
        }
        return nullptr;
    }

    static bool RenderPropertiesForm(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick);
    static bool RenderPropertyWidget(const std::string& label, const std::string& propName,
        const std::string& widgetType,
        Engine::PropertyVariant& propVariant, const nlohmann::json& propSchema,
        const Engine::PropertyMap& allProperties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick);
    static bool RenderTable(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick);
    static bool RenderTableWidget(const std::string& widgetType, Engine::PropertyVariant& propVariant,
        const nlohmann::json& propSchema, const std::string& propName,
        const Engine::PropertyMap& allProperties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick);
    static bool RenderSeparateWindows(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick);

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

    static bool RenderSchema(const nlohmann::json& schema, Engine::PropertyMap& properties,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick = nullptr,
        const std::string& componentName = "", int entityId = 0,
        const std::unordered_map<std::string, std::string>* pathMap = nullptr) {
        if (!schema.is_object()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid schema: not an object");
            return false;
        }

        bool fileDialogModified = FileDialogWidgets::ProcessDialog();
        bool mediaLoadModified = MediaLoadingWidgets::ProcessDialog();

        PushStyleFromSchema(schema);
        bool modified = false;

        UIRenderContext context(componentName, entityId, pathMap);

        if (schema.contains("ui:separate_windows") && schema["ui:separate_windows"].get<bool>()) {
            modified = RenderSeparateWindows(schema, properties, context, onPropertyRightClick);
        }
        else if (schema.contains("ui:table") && schema["ui:table"].is_object()) {
            modified = RenderTable(schema, properties, context, onPropertyRightClick);
        }
        else if (schema.contains("properties") && schema["properties"].is_object()) {
            modified = RenderPropertiesForm(schema, properties, context, onPropertyRightClick);
        }

        PopStyleFromSchema(schema);

        return modified || fileDialogModified || mediaLoadModified || FileDialogWidgets::WasModified() || MediaLoadingWidgets::WasModified();
    }

    static bool RenderSeparateWindows(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick) {
        bool modified = false;
        auto& windowStates = GetWindowStates();

        if (!schema.contains("properties") || !schema["properties"].is_object()) {
            return false;
        }

        std::vector<std::string> propertyOrder = GetPropertyOrder(schema);

        for (const auto& propName : propertyOrder) {
            if (schema["properties"].contains(propName)) {
                const auto& propSchema = schema["properties"][propName];

                if (propSchema.contains("ui:window") && propSchema["ui:window"].get<bool>()) {
                    if (properties.find(propName) == properties.end()) {
                        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Property not found: %s", propName.c_str());
                        continue;
                    }

                    std::string windowName = propName;
                    if (propSchema.contains("ui:window_name") && propSchema["ui:window_name"].is_string()) {
                        windowName = propSchema["ui:window_name"].get<std::string>();
                    }

                    std::string windowId = context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber) + "_window_" + windowName;

                    if (windowStates.find(windowId) == windowStates.end()) {
                        windowStates[windowId] = false;
                    }

                    std::string checkboxLabel = windowName + " Editor##" + context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber);
                    ImGui::Checkbox(checkboxLabel.c_str(), &windowStates[windowId]);
                    RenderTooltipIfPresent(propSchema);

                    ImGui::SameLine();
                    if (std::holds_alternative<std::string*>(properties[propName])) {
                        std::string* strPtr = std::get<std::string*>(properties[propName]);
                        ImGui::Text("(%zu chars)", strPtr->length());
                    }

                    if (windowStates[windowId]) {
                        bool& showWindow = windowStates[windowId];
                        if (ImGui::Begin(windowId.c_str(), &showWindow)) {
                            std::string widgetType = GetWidgetType(propSchema);
                            Engine::PropertyVariant& propVariant = properties[propName];
                            try {
                                std::string uniqueLabel = "##" + context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber);
                                if (RenderPropertyWidget(uniqueLabel, propName, widgetType, propVariant, propSchema, properties, context, onPropertyRightClick)) {
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
                    std::string widgetType = GetWidgetType(propSchema);
                    std::string label = GetPropertyLabel(propName, propSchema);
                    Engine::PropertyVariant& propVariant = properties[propName];
                    try {
                        std::string uniqueLabel = label + "##" + context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber);
                        if (RenderPropertyWidget(uniqueLabel, propName, widgetType, propVariant, propSchema, properties, context, onPropertyRightClick)) {
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

    static bool RenderPropertiesForm(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick) {
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
                Engine::PropertyVariant& propVariant = properties[propName];

                try {
                    std::string uniqueLabel = label + "##" + context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber);
                    if (RenderPropertyWidget(uniqueLabel, propName, widgetType, propVariant, propSchema, properties, context, onPropertyRightClick)) {
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

    static bool RenderPropertyWidget(const std::string& label, const std::string& propName,
        const std::string& widgetType,
        Engine::PropertyVariant& propVariant, const nlohmann::json& propSchema,
        const Engine::PropertyMap& allProperties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick) {
        bool modified = false;

        if (std::holds_alternative<bool*>(propVariant)) {
            bool* value = std::get<bool*>(propVariant);
            modified = BoolWidgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<int*>(propVariant)) {
            int* value = std::get<int*>(propVariant);
            modified = IntWidgets::Render(label, value, widgetType, propSchema, allProperties, context);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<int64_t*>(propVariant)) {
            int64_t* value = std::get<int64_t*>(propVariant);
            modified = IntWidgets::Render(label, value, widgetType, propSchema, allProperties, context);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<void*>(propVariant)) {
            void* ptr = std::get<void*>(propVariant);
            int64_t* value = reinterpret_cast<int64_t*>(ptr);
            modified = IntWidgets::Render(label, value, widgetType, propSchema, allProperties, context);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<float*>(propVariant)) {
            float* value = std::get<float*>(propVariant);
            modified = FloatWidgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<double*>(propVariant)) {
            double* value = std::get<double*>(propVariant);
            modified = DoubleWidgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<std::string*>(propVariant)) {
            std::string* value = std::get<std::string*>(propVariant);
            if (widgetType == "file_selector") {
                modified = FileDialogWidgets::Render(label, value, widgetType, propSchema, context);
            }
            else if (widgetType == "media_loader") {
                modified = MediaLoadingWidgets::Render(label, widgetType, propSchema, allProperties, context);
            }
            else {
                std::string propNameClean = propName;
                std::string originalLabel = label;
                size_t hashPos = label.find("##");
                if (hashPos != std::string::npos) {
                    originalLabel = label.substr(0, hashPos);
                }
                nlohmann::json modifiedSchema = propSchema;
                modifiedSchema["ui:displayName"] = originalLabel;
                std::string stringWidgetLabel = "##" + context.componentName + "_" + propNameClean + "_" + std::to_string(context.entityNumber);
                modified = StringWidgets::Render(stringWidgetLabel, value, widgetType, modifiedSchema, context);
            }
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<ImVec2*>(propVariant)) {
            ImVec2* value = std::get<ImVec2*>(propVariant);
            modified = Vec2Widgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<ImVec4*>(propVariant)) {
            ImVec4* value = std::get<ImVec4*>(propVariant);
            modified = Vec4Widgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else if (std::holds_alternative<std::vector<std::string>*>(propVariant)) {
            std::vector<std::string>* value = std::get<std::vector<std::string>*>(propVariant);
            modified = StringArrayWidgets::Render(label, value, widgetType, propSchema);
            RenderTooltipIfPresent(propSchema);
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Unsupported property type");
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            if (onPropertyRightClick) {
                nlohmann::json newValue = GetPropertyValueAsJson(propVariant);
                onPropertyRightClick(propName, newValue);
            }
        }

        return modified;
    }

    static bool RenderTable(const nlohmann::json& schema, Engine::PropertyMap& properties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick) {
        if (!schema.contains("ui:table") || !schema["ui:table"].is_object()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid table schema");
            return false;
        }

        const auto& tableSchema = schema["ui:table"];
        int columns = GetSchemaValue<int>(tableSchema, "columns", 2);
        ImGuiTableFlags flags = GetTableFlags(tableSchema);

        std::string tableId = context.componentName + "_table_" + std::to_string(context.entityNumber);
        bool tableCreated = ImGui::BeginTable(tableId.c_str(), columns, flags);
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
                    RenderTooltipIfPresent(propSchema);

                    ImGui::TableNextColumn();
                    std::string widgetType = GetWidgetType(propSchema);
                    Engine::PropertyVariant& propVariant = properties[propName];

                    std::string pushId = context.componentName + "_" + propName + "_" + std::to_string(context.entityNumber) + "_table";
                    ImGui::PushID(pushId.c_str());

                    try {
                        if (RenderTableWidget(widgetType, propVariant, propSchema, propName, properties, context, onPropertyRightClick)) {
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

    static bool RenderTableWidget(const std::string& widgetType, Engine::PropertyVariant& propVariant,
        const nlohmann::json& propSchema, const std::string& propName,
        const Engine::PropertyMap& allProperties,
        const UIRenderContext& context,
        std::function<void(const std::string& propName, const nlohmann::json& value)> onPropertyRightClick) {
        std::string uniqueLabel = "##" + context.componentName + "_value_" + std::to_string(context.entityNumber);
        return RenderPropertyWidget(uniqueLabel, propName, widgetType, propVariant, propSchema, allProperties, context, onPropertyRightClick);
    }

} // namespace UISchema