#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>

namespace ECS {

    class ImGuiStyleSettingsComponent : public BaseComponent {
    public:
        ImGuiStyleSettingsComponent() = default;

        const char* GetCompName() const override { return "ImGuiStyleSettingsComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "ImGui Style Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "WindowRounding", "ChildRounding", "FrameRounding", "PopupRounding",
                    "ScrollbarRounding", "GrabRounding", "TabRounding",
                    "WindowBorderSize", "ChildBorderSize", "PopupBorderSize",
                    "FrameBorderSize", "TabBorderSize",
                    "WindowPadding", "FramePadding", "ItemSpacing", "ItemInnerSpacing",
                    "IndentSpacing", "ScrollbarSize", "GrabMinSize"
                }},
                {"properties", {
                    {"WindowRounding", {{"type","number"},{"title","Window Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"ChildRounding", {{"type","number"},{"title","Child Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"FrameRounding", {{"type","number"},{"title","Frame Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"PopupRounding", {{"type","number"},{"title","Popup Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"ScrollbarRounding", {{"type","number"},{"title","Scrollbar Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"GrabRounding", {{"type","number"},{"title","Grab Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"TabRounding", {{"type","number"},{"title","Tab Rounding"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",12.0f},{"format","%.1f"}}}}},
                    {"WindowBorderSize", {{"type","number"},{"title","Window Border Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",1.0f},{"format","%.1f"}}}}},
                    {"ChildBorderSize", {{"type","number"},{"title","Child Border Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",1.0f},{"format","%.1f"}}}}},
                    {"PopupBorderSize", {{"type","number"},{"title","Popup Border Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",1.0f},{"format","%.1f"}}}}},
                    {"FrameBorderSize", {{"type","number"},{"title","Frame Border Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",1.0f},{"format","%.1f"}}}}},
                    {"TabBorderSize", {{"type","number"},{"title","Tab Border Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.1f},{"min",0.0f},{"max",1.0f},{"format","%.1f"}}}}},
                    {"WindowPadding", {{"type","array"},{"title","Window Padding"},{"ui:widget","input_vec2"}}},
                    {"FramePadding", {{"type","array"},{"title","Frame Padding"},{"ui:widget","input_vec2"}}},
                    {"ItemSpacing", {{"type","array"},{"title","Item Spacing"},{"ui:widget","input_vec2"}}},
                    {"ItemInnerSpacing", {{"type","array"},{"title","Item Inner Spacing"},{"ui:widget","input_vec2"}}},
                    {"IndentSpacing", {{"type","number"},{"title","Indent Spacing"},{"ui:widget","input_float"},{"ui:options",{{"step",0.5f},{"min",0.0f},{"max",30.0f},{"format","%.1f"}}}}},
                    {"ScrollbarSize", {{"type","number"},{"title","Scrollbar Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.5f},{"min",1.0f},{"max",20.0f},{"format","%.1f"}}}}},
                    {"GrabMinSize", {{"type","number"},{"title","Grab Min Size"},{"ui:widget","input_float"},{"ui:options",{{"step",0.5f},{"min",1.0f},{"max",20.0f},{"format","%.1f"}}}}}
                }}
            };
            return j;
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"WindowPadding", &currentStyle.WindowPadding},
                {"FramePadding", &currentStyle.FramePadding},
                {"ItemSpacing", &currentStyle.ItemSpacing},
                {"ItemInnerSpacing", &currentStyle.ItemInnerSpacing},
                {"IndentSpacing", &currentStyle.IndentSpacing},
                {"ScrollbarSize", &currentStyle.ScrollbarSize},
                {"GrabMinSize", &currentStyle.GrabMinSize},
                {"WindowBorderSize", &currentStyle.WindowBorderSize},
                {"ChildBorderSize", &currentStyle.ChildBorderSize},
                {"PopupBorderSize", &currentStyle.PopupBorderSize},
                {"FrameBorderSize", &currentStyle.FrameBorderSize},
                {"TabBorderSize", &currentStyle.TabBorderSize},
                {"WindowRounding", &currentStyle.WindowRounding},
                {"ChildRounding", &currentStyle.ChildRounding},
                {"FrameRounding", &currentStyle.FrameRounding},
                {"PopupRounding", &currentStyle.PopupRounding},
                {"ScrollbarRounding", &currentStyle.ScrollbarRounding},
                {"GrabRounding", &currentStyle.GrabRounding},
                {"TabRounding", &currentStyle.TabRounding}
            };
        }

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void ResetToDefaults();

        ImGuiStyle currentStyle;
    };

}