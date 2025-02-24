// node_utils.hpp
#pragma once
#include <imgui.h>
#include <imgui_node_editor.h>

namespace NodeEditorUtils {

// Layout helpers specifically for node editor
inline void Spring(float weight = 1.0f) {
    auto &style = ax::NodeEditor::GetStyle();
    ImGui::Dummy(ImVec2(weight * style.NodePadding.x, 0));
    ImGui::SameLine();
}

inline void BeginHorizontal(const char *id) {
    ImGui::BeginGroup();
    ImGui::PushID(id);
}

inline void EndHorizontal() {
    ImGui::PopID();
    ImGui::EndGroup();
}

inline void BeginVertical(const char *id) {
    ImGui::BeginGroup();
    ImGui::PushID(id);
}

inline void EndVertical() {
    ImGui::PopID();
    ImGui::EndGroup();
}

} // namespace NodeEditorUtils