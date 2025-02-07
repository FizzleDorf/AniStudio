#include <pch.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void SaveStyleToFile(const ImGuiStyle &style, const std::string &filename) {
    json j;

    // Save window settings
    j["WindowPadding"] = {style.WindowPadding.x, style.WindowPadding.y};
    j["FramePadding"] = {style.FramePadding.x, style.FramePadding.y};
    j["ItemSpacing"] = {style.ItemSpacing.x, style.ItemSpacing.y};
    j["ScrollbarSize"] = style.ScrollbarSize;
    j["WindowBorderSize"] = style.WindowBorderSize;
    j["FrameBorderSize"] = style.FrameBorderSize;
    j["FrameRounding"] = style.FrameRounding;
    j["WindowRounding"] = style.WindowRounding;

    // Save colors
    j["Colors"] = json::array();
    for (int i = 0; i < ImGuiCol_COUNT; i++) {
        const ImVec4 &col = style.Colors[i];
        j["Colors"].push_back(
            {{"name", ImGui::GetStyleColorName(i)}, {"r", col.x}, {"g", col.y}, {"b", col.z}, {"a", col.w}});
    }

    // Write to file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
}

static void LoadStyleFromFile(ImGuiStyle &style, const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        return;

    json j;
    file >> j;

    // Load window settings
    if (j.contains("WindowPadding"))
        style.WindowPadding = ImVec2(j["WindowPadding"][0], j["WindowPadding"][1]);
    if (j.contains("FramePadding"))
        style.FramePadding = ImVec2(j["FramePadding"][0], j["FramePadding"][1]);
    if (j.contains("ItemSpacing"))
        style.ItemSpacing = ImVec2(j["ItemSpacing"][0], j["ItemSpacing"][1]);
    if (j.contains("ScrollbarSize"))
        style.ScrollbarSize = j["ScrollbarSize"];
    if (j.contains("WindowBorderSize"))
        style.WindowBorderSize = j["WindowBorderSize"];
    if (j.contains("FrameBorderSize"))
        style.FrameBorderSize = j["FrameBorderSize"];
    if (j.contains("FrameRounding"))
        style.FrameRounding = j["FrameRounding"];
    if (j.contains("WindowRounding"))
        style.WindowRounding = j["WindowRounding"];

    // Load colors
    if (j.contains("Colors")) {
        for (const auto &color : j["Colors"]) {
            for (int i = 0; i < ImGuiCol_COUNT; i++) {
                if (color["name"] == ImGui::GetStyleColorName(i)) {
                    style.Colors[i] = ImVec4(color["r"], color["g"], color["b"], color["a"]);
                    break;
                }
            }
        }
    }
}