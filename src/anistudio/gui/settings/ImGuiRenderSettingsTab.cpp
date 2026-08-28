#include "ImGuiRenderSettingsTab.hpp"
#include <imgui_internal.h>
#include <algorithm>

namespace ECS {

    ImGuiRenderSettingsTab::ImGuiRenderSettingsTab(ImGuiRenderSettingsComponent& comp) : m_comp(comp) {}

    bool ImGuiRenderSettingsTab::FilterPass(const std::string& section) const {
        if (m_filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = m_filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void ImGuiRenderSettingsTab::Render() {
        if (!m_comp.imguiContext) return;
        m_comp.EnsureInitialized();
        ImGui::SetCurrentContext(m_comp.imguiContext);

        if (ImGui::BeginChild("ImGuiRenderSettings", ImVec2(0, 0), false)) {
            ImGuiIO& io = ImGui::GetIO();

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: Some changes require application restart");
            ImGui::Separator();

            if (FilterPass("Input Settings")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Input Settings")) RenderInputSettings();
            }
            if (FilterPass("Window Behavior")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Window Behavior")) RenderWindowBehavior();
            }
            if (FilterPass("Navigation Settings")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Navigation Settings")) RenderNavigation();
            }
            if (FilterPass("Docking Settings")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Docking Settings")) RenderDocking();
            }
            if (FilterPass("Multi-Viewport Settings")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Multi-Viewport Settings")) RenderViewports();
            }
            if (FilterPass("Memory & Performance")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Memory & Performance")) RenderMemory();
            }
            if (FilterPass("Input Text Settings")) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
                if (ImGui::CollapsingHeader("Input Text Settings")) RenderInputText();
            }

            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void ImGuiRenderSettingsTab::RenderInputSettings() {
        ImGuiIO& io = ImGui::GetIO();
        float doubleClickTime = io.MouseDoubleClickTime;
        if (ImGui::SliderFloat("Mouse Double Click Time", &doubleClickTime, 0.1f, 1.0f, "%.2f")) {
            io.MouseDoubleClickTime = doubleClickTime;
            m_comp.hasChanges = true;
        }
        float dragThreshold = io.MouseDragThreshold;
        if (ImGui::SliderFloat("Mouse Drag Threshold", &dragThreshold, 0.0f, 20.0f, "%.1f")) {
            io.MouseDragThreshold = dragThreshold;
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderWindowBehavior() {
        bool changed = false;
        changed |= ImGui::Checkbox("Windows Resize From Edges", &m_comp.configWindowsResizeFromEdges);
        changed |= ImGui::Checkbox("Windows Move From Title Bar Only", &m_comp.configWindowsMoveFromTitleBarOnly);
        changed |= ImGui::Checkbox("Drag Click to Input Text", &m_comp.configDragClickToInputText);
        if (changed) {
            m_comp.ApplyWindowBehaviorToImGui();
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderNavigation() {
        bool changed = false;
        changed |= ImGui::Checkbox("Enable Keyboard Navigation", &m_comp.configNavEnableKeyboard);
        changed |= ImGui::Checkbox("Enable Gamepad Navigation", &m_comp.configNavEnableGamepad);
        changed |= ImGui::Checkbox("Nav Move Set Mouse Pos", &m_comp.configNavMoveSetMousePos);
        changed |= ImGui::Checkbox("Nav Capture Keyboard", &m_comp.configNavCaptureKeyboard);
        changed |= ImGui::Checkbox("Nav Escape Clear Focus Item", &m_comp.configNavEscapeClearFocusItem);
        if (changed) {
            m_comp.ApplyNavigationToImGui();
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderDocking() {
        bool changed = false;
        changed |= ImGui::Checkbox("Enable Docking", &m_comp.configDockingEnable);
        if (m_comp.configDockingEnable) {
            ImGui::Indent();
            changed |= ImGui::Checkbox("Docking With Shift", &m_comp.configDockingWithShift);
            changed |= ImGui::Checkbox("Docking Always Tab Bar", &m_comp.configDockingAlwaysTabBar);
            changed |= ImGui::Checkbox("Docking Transparent Payload", &m_comp.configDockingTransparentPayload);
            ImGui::Unindent();
        }
        if (changed) {
            m_comp.ApplyDockingToImGui();
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderViewports() {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Warning: Viewports may cause performance issues");
        bool changed = false;
        changed |= ImGui::Checkbox("Enable Viewports", &m_comp.configViewportsEnable);
        if (m_comp.configViewportsEnable) {
            ImGui::Indent();
            changed |= ImGui::Checkbox("Viewports No Auto Merge", &m_comp.configViewportsNoAutoMerge);
            changed |= ImGui::Checkbox("Viewports No Task Bar Icon", &m_comp.configViewportsNoTaskBarIcon);
            changed |= ImGui::Checkbox("Viewports No Decoration", &m_comp.configViewportsNoDecoration);
            changed |= ImGui::Checkbox("Viewports No Default Parent", &m_comp.configViewportsNoDefaultParent);
            ImGui::Unindent();
        }
        if (changed) {
            m_comp.ApplyViewportsToImGui();
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderMemory() {
        bool memoryCompactEnabled = (m_comp.configMemoryCompactTimer >= 0.0f);
        if (ImGui::Checkbox("Memory Compact Timer", &memoryCompactEnabled)) {
            m_comp.configMemoryCompactTimer = memoryCompactEnabled ? 60.0f : -1.0f;
            ImGui::GetIO().ConfigMemoryCompactTimer = m_comp.configMemoryCompactTimer;
            m_comp.hasChanges = true;
        }
        if (m_comp.configMemoryCompactTimer >= 0.0f) {
            ImGui::SameLine();
            if (ImGui::SliderFloat("##Timer", &m_comp.configMemoryCompactTimer, 10.0f, 300.0f, "%.0fs")) {
                ImGui::GetIO().ConfigMemoryCompactTimer = m_comp.configMemoryCompactTimer;
                m_comp.hasChanges = true;
            }
        }
    }

    void ImGuiRenderSettingsTab::RenderInputText() {
        bool changed = false;
        changed |= ImGui::Checkbox("Input Text Cursor Blink", &m_comp.configInputTextCursorBlink);
        changed |= ImGui::Checkbox("Input Text Enter Keep Active", &m_comp.configInputTextEnterKeepActive);
        if (changed) {
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigInputTextCursorBlink = m_comp.configInputTextCursorBlink;
            io.ConfigInputTextEnterKeepActive = m_comp.configInputTextEnterKeepActive;
            m_comp.hasChanges = true;
        }
    }

    void ImGuiRenderSettingsTab::RenderActionButtons() {
        if (ImGui::Button("Apply Settings")) {
            m_comp.ApplyAllSettingsToImGui();
            SaveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (m_comp.HasUnsavedChanges()) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
    }

}