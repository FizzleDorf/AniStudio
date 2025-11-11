#pragma once

#include <pch.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static void SetCustomDarkTheme() {
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.30f, 0.60f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.30f, 0.32f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.42f, 0.52f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.44f, 0.54f, 0.64f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.38f, 0.48f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.24f, 0.34f, 0.44f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.36f, 0.46f, 0.56f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.40f, 0.50f, 0.60f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.30f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.24f, 0.25f, 0.26f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.34f, 0.44f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.46f, 0.56f, 0.66f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	style->WindowPadding = ImVec2(8.00f, 8.00f);
	style->FramePadding = ImVec2(5.00f, 2.00f);
	style->CellPadding = ImVec2(6.00f, 6.00f);
	style->ItemSpacing = ImVec2(6.00f, 6.00f);
	style->ItemInnerSpacing = ImVec2(6.00f, 6.00f);
	style->TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style->IndentSpacing = 25;
	style->ScrollbarSize = 11;
	style->GrabMinSize = 10;
	style->WindowBorderSize = 1;
	style->ChildBorderSize = 1;
	style->PopupBorderSize = 1;
	style->FrameBorderSize = 1;
	style->TabBorderSize = 1;
	style->WindowRounding = 7;
	style->ChildRounding = 4;
	style->FrameRounding = 3;
	style->PopupRounding = 4;
	style->ScrollbarRounding = 9;
	style->GrabRounding = 3;
	style->LogSliderDeadzone = 4;
	style->TabRounding = 4;
}

static void SaveStyleToFile(const ImGuiStyle &style, const std::string &filename) {
	json j;

	j["WindowPadding"] = { style.WindowPadding.x, style.WindowPadding.y };
	j["FramePadding"] = { style.FramePadding.x, style.FramePadding.y };
	j["ItemSpacing"] = { style.ItemSpacing.x, style.ItemSpacing.y };
	j["ScrollbarSize"] = style.ScrollbarSize;
	j["WindowBorderSize"] = style.WindowBorderSize;
	j["FrameBorderSize"] = style.FrameBorderSize;
	j["FrameRounding"] = style.FrameRounding;
	j["WindowRounding"] = style.WindowRounding;

	j["Colors"] = json::array();
	for (int i = 0; i < ImGuiCol_COUNT; i++) {
		const ImVec4 &col = style.Colors[i];
		j["Colors"].push_back(
			{ {"name", ImGui::GetStyleColorName(i)}, {"r", col.x}, {"g", col.y}, {"b", col.z}, {"a", col.w} });
	}

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