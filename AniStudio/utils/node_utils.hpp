#pragma once
#include <imgui.h>
#include <imgui_node_editor.h>

namespace ax {
namespace NodeEditor {
namespace Utilities {

// Layout helpers specifically for node editor
inline void Spring(float weight = 1.0f) {
    auto &style = ed::GetStyle();
    ImGui::ItemSize(ImVec2(0, 0));
    ImGui::SameLine();
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

} // namespace Utilities
} // namespace NodeEditor
} // namespace ax