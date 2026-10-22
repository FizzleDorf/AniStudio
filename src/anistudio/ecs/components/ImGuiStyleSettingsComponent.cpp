#include "ImGuiStyleSettingsComponent.hpp"

namespace ECS {

    nlohmann::json ImGuiStyleSettingsComponent::Serialize() const {
        const ImGuiStyle& style = currentStyle;
        nlohmann::json j;
        j["WindowPadding"] = { style.WindowPadding.x, style.WindowPadding.y };
        j["FramePadding"] = { style.FramePadding.x, style.FramePadding.y };
        j["ItemSpacing"] = { style.ItemSpacing.x, style.ItemSpacing.y };
        j["ItemInnerSpacing"] = { style.ItemInnerSpacing.x, style.ItemInnerSpacing.y };
        j["IndentSpacing"] = style.IndentSpacing;
        j["ScrollbarSize"] = style.ScrollbarSize;
        j["GrabMinSize"] = style.GrabMinSize;
        j["WindowBorderSize"] = style.WindowBorderSize;
        j["ChildBorderSize"] = style.ChildBorderSize;
        j["PopupBorderSize"] = style.PopupBorderSize;
        j["FrameBorderSize"] = style.FrameBorderSize;
        j["TabBorderSize"] = style.TabBorderSize;
        j["WindowRounding"] = style.WindowRounding;
        j["ChildRounding"] = style.ChildRounding;
        j["FrameRounding"] = style.FrameRounding;
        j["PopupRounding"] = style.PopupRounding;
        j["ScrollbarRounding"] = style.ScrollbarRounding;
        j["GrabRounding"] = style.GrabRounding;
        j["TabRounding"] = style.TabRounding;
        j["Colors"] = nlohmann::json::array();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            const ImVec4& col = style.Colors[i];
            j["Colors"].push_back({
                {"name", ImGui::GetStyleColorName(i)},
                {"r", col.x}, {"g", col.y}, {"b", col.z}, {"a", col.w}
                });
        }
        return j;
    }

    void ImGuiStyleSettingsComponent::Deserialize(const nlohmann::json& j) {
        ImGuiStyle& style = currentStyle;
        if (j.contains("WindowPadding")) style.WindowPadding = ImVec2(j["WindowPadding"][0], j["WindowPadding"][1]);
        if (j.contains("FramePadding")) style.FramePadding = ImVec2(j["FramePadding"][0], j["FramePadding"][1]);
        if (j.contains("ItemSpacing")) style.ItemSpacing = ImVec2(j["ItemSpacing"][0], j["ItemSpacing"][1]);
        if (j.contains("ItemInnerSpacing")) style.ItemInnerSpacing = ImVec2(j["ItemInnerSpacing"][0], j["ItemInnerSpacing"][1]);
        if (j.contains("IndentSpacing")) style.IndentSpacing = j["IndentSpacing"];
        if (j.contains("ScrollbarSize")) style.ScrollbarSize = j["ScrollbarSize"];
        if (j.contains("GrabMinSize")) style.GrabMinSize = j["GrabMinSize"];
        if (j.contains("WindowBorderSize")) style.WindowBorderSize = j["WindowBorderSize"];
        if (j.contains("ChildBorderSize")) style.ChildBorderSize = j["ChildBorderSize"];
        if (j.contains("PopupBorderSize")) style.PopupBorderSize = j["PopupBorderSize"];
        if (j.contains("FrameBorderSize")) style.FrameBorderSize = j["FrameBorderSize"];
        if (j.contains("TabBorderSize")) style.TabBorderSize = j["TabBorderSize"];
        if (j.contains("WindowRounding")) style.WindowRounding = j["WindowRounding"];
        if (j.contains("ChildRounding")) style.ChildRounding = j["ChildRounding"];
        if (j.contains("FrameRounding")) style.FrameRounding = j["FrameRounding"];
        if (j.contains("PopupRounding")) style.PopupRounding = j["PopupRounding"];
        if (j.contains("ScrollbarRounding")) style.ScrollbarRounding = j["ScrollbarRounding"];
        if (j.contains("GrabRounding")) style.GrabRounding = j["GrabRounding"];
        if (j.contains("TabRounding")) style.TabRounding = j["TabRounding"];
        if (j.contains("Colors")) {
            for (const auto& color : j["Colors"]) {
                for (int i = 0; i < ImGuiCol_COUNT; i++) {
                    if (color["name"] == ImGui::GetStyleColorName(i)) {
                        style.Colors[i] = ImVec4(color["r"], color["g"], color["b"], color["a"]);
                        break;
                    }
                }
            }
        }
    }

    void ImGuiStyleSettingsComponent::ResetToDefaults() {
        currentStyle = ImGuiStyle{};
    }

}