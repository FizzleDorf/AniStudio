#include "ImGuiRenderSettingsComponent.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace ECS {

    ImGuiRenderSettingsComponent::ImGuiRenderSettingsComponent() {
        compName = "ImGuiRenderSettingsComponent";
    }

    bool ImGuiRenderSettingsComponent::FilterPass(const std::string& section, const std::string& filter) const {
        if (filter.empty()) return true;
        std::string lower = section;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void ImGuiRenderSettingsComponent::EnsureInitialized() {
        if (!isInitialized && imguiContext) {
            ImGui::SetCurrentContext(imguiContext);
            LoadCurrentImGuiSettings();
            isInitialized = true;
        }
    }

    void ImGuiRenderSettingsComponent::RenderUI() {
        RenderFilteredUI("");
    }

    void ImGuiRenderSettingsComponent::RenderFilteredUI(const std::string& filter) {
        if (!imguiContext) return;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);

        if (ImGui::BeginChild("ImGuiRenderSettings", ImVec2(0, 0), false)) {
            ImGuiIO& io = ImGui::GetIO();

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Warning: Some changes require application restart");
            ImGui::Separator();

            if (FilterPass("Display Settings", filter)) {
                float fontScale = io.FontGlobalScale;
                if (ImGui::SliderFloat("Global Font Scale", &fontScale, 0.5f, 2.0f, "%.2f")) {
                    io.FontGlobalScale = fontScale;
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Input Settings", filter)) {
                float doubleClickTime = io.MouseDoubleClickTime;
                if (ImGui::SliderFloat("Mouse Double Click Time", &doubleClickTime, 0.1f, 1.0f, "%.2f")) {
                    io.MouseDoubleClickTime = doubleClickTime;
                    hasChanges = true;
                }
                float dragThreshold = io.MouseDragThreshold;
                if (ImGui::SliderFloat("Mouse Drag Threshold", &dragThreshold, 0.0f, 20.0f, "%.1f")) {
                    io.MouseDragThreshold = dragThreshold;
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Window Behavior", filter)) {
                bool changed = false;
                changed |= ImGui::Checkbox("Windows Resize From Edges", &configWindowsResizeFromEdges);
                changed |= ImGui::Checkbox("Windows Move From Title Bar Only", &configWindowsMoveFromTitleBarOnly);
                changed |= ImGui::Checkbox("Drag Click to Input Text", &configDragClickToInputText);
                if (changed) {
                    ApplyWindowBehaviorToImGui();
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Navigation Settings", filter)) {
                bool changed = false;
                changed |= ImGui::Checkbox("Enable Keyboard Navigation", &configNavEnableKeyboard);
                changed |= ImGui::Checkbox("Enable Gamepad Navigation", &configNavEnableGamepad);
                changed |= ImGui::Checkbox("Nav Move Set Mouse Pos", &configNavMoveSetMousePos);
                changed |= ImGui::Checkbox("Nav Capture Keyboard", &configNavCaptureKeyboard);
                changed |= ImGui::Checkbox("Nav Escape Clear Focus Item", &configNavEscapeClearFocusItem);
                if (changed) {
                    ApplyNavigationToImGui();
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Docking Settings", filter)) {
                bool changed = false;
                changed |= ImGui::Checkbox("Enable Docking", &configDockingEnable);
                if (configDockingEnable) {
                    ImGui::Indent();
                    changed |= ImGui::Checkbox("Docking With Shift", &configDockingWithShift);
                    changed |= ImGui::Checkbox("Docking Always Tab Bar", &configDockingAlwaysTabBar);
                    changed |= ImGui::Checkbox("Docking Transparent Payload", &configDockingTransparentPayload);
                    ImGui::Unindent();
                }
                if (changed) {
                    ApplyDockingToImGui();
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Multi-Viewport Settings", filter)) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Warning: Viewports may cause performance issues");
                bool changed = false;
                changed |= ImGui::Checkbox("Enable Viewports", &configViewportsEnable);
                if (configViewportsEnable) {
                    ImGui::Indent();
                    changed |= ImGui::Checkbox("Viewports No Auto Merge", &configViewportsNoAutoMerge);
                    changed |= ImGui::Checkbox("Viewports No Task Bar Icon", &configViewportsNoTaskBarIcon);
                    changed |= ImGui::Checkbox("Viewports No Decoration", &configViewportsNoDecoration);
                    changed |= ImGui::Checkbox("Viewports No Default Parent", &configViewportsNoDefaultParent);
                    ImGui::Unindent();
                }
                if (changed) {
                    ApplyViewportsToImGui();
                    hasChanges = true;
                }
                ImGui::Separator();
            }
            if (FilterPass("Memory & Performance", filter)) {
                bool memoryCompactEnabled = (configMemoryCompactTimer >= 0.0f);
                if (ImGui::Checkbox("Memory Compact Timer", &memoryCompactEnabled)) {
                    configMemoryCompactTimer = memoryCompactEnabled ? 60.0f : -1.0f;
                    io.ConfigMemoryCompactTimer = configMemoryCompactTimer;
                    hasChanges = true;
                }
                if (configMemoryCompactTimer >= 0.0f) {
                    ImGui::SameLine();
                    if (ImGui::SliderFloat("##Timer", &configMemoryCompactTimer, 10.0f, 300.0f, "%.0fs")) {
                        io.ConfigMemoryCompactTimer = configMemoryCompactTimer;
                        hasChanges = true;
                    }
                }
                ImGui::Separator();
            }
            if (FilterPass("Input Text Settings", filter)) {
                bool changed = false;
                changed |= ImGui::Checkbox("Input Text Cursor Blink", &configInputTextCursorBlink);
                changed |= ImGui::Checkbox("Input Text Enter Keep Active", &configInputTextEnterKeepActive);
                if (changed) {
                    io.ConfigInputTextCursorBlink = configInputTextCursorBlink;
                    io.ConfigInputTextEnterKeepActive = configInputTextEnterKeepActive;
                    hasChanges = true;
                }
                ImGui::Separator();
            }

            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    bool ImGuiRenderSettingsComponent::SaveSettings() {
        if (!imguiContext) return false;
        EnsureInitialized();
        try {
            nlohmann::json j;
            SerializeSettings(j);
            std::string filePath = GetSettingsDirectory() + "/imgui_render_settings.json";
            std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
            std::ofstream file(filePath);
            if (!file.is_open()) return false;
            file << j.dump(4);
            file.close();
            hasChanges = false;
            CreateBackup();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ImGuiRenderSettingsComponent] Save error: " << e.what() << std::endl;
            return false;
        }
    }

    bool ImGuiRenderSettingsComponent::LoadSettings() {
        if (!imguiContext) return false;
        EnsureInitialized();
        try {
            std::string filePath = GetSettingsDirectory() + "/imgui_render_settings.json";
            if (!std::filesystem::exists(filePath)) {
                LoadDefaults();
                return true;
            }
            std::ifstream file(filePath);
            if (!file.is_open()) return false;
            nlohmann::json j;
            file >> j;
            file.close();
            DeserializeSettings(j);
            ApplyAllSettingsToImGui();
            hasChanges = false;
            CreateBackup();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[ImGuiRenderSettingsComponent] Load error: " << e.what() << std::endl;
            return false;
        }
    }

    void ImGuiRenderSettingsComponent::ResetToDefaults() {
        if (!imguiContext) return;
        EnsureInitialized();
        LoadDefaults();
        ApplyAllSettingsToImGui();
        hasChanges = true;
    }

    void ImGuiRenderSettingsComponent::CreateBackup() {
        if (!imguiContext) return;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);
        backupConfigFlags = ImGui::GetIO().ConfigFlags;
    }

    void ImGuiRenderSettingsComponent::RestoreFromBackup() {
        if (!imguiContext) return;
        EnsureInitialized();
        ImGui::SetCurrentContext(imguiContext);
        ImGui::GetIO().ConfigFlags = backupConfigFlags;
        LoadCurrentImGuiSettings();
        hasChanges = false;
    }

    void ImGuiRenderSettingsComponent::LoadCurrentImGuiSettings() {
        if (!imguiContext) return;
        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();
        configWindowsResizeFromEdges = io.ConfigWindowsResizeFromEdges;
        configWindowsMoveFromTitleBarOnly = io.ConfigWindowsMoveFromTitleBarOnly;
        configDragClickToInputText = io.ConfigDragClickToInputText;
        configNavEnableKeyboard = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
        configNavEnableGamepad = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0;
        configNavMoveSetMousePos = io.ConfigNavMoveSetMousePos;
        configNavCaptureKeyboard = io.ConfigNavCaptureKeyboard;
        configNavEscapeClearFocusItem = io.ConfigNavEscapeClearFocusItem;
        configMemoryCompactTimer = io.ConfigMemoryCompactTimer;
        configDebugHighlightIdConflicts = io.ConfigDebugHighlightIdConflicts;
        configDockingEnable = (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0;
        configDockingWithShift = io.ConfigDockingWithShift;
        configDockingAlwaysTabBar = io.ConfigDockingAlwaysTabBar;
        configDockingTransparentPayload = io.ConfigDockingTransparentPayload;
        configViewportsEnable = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
        configViewportsNoAutoMerge = io.ConfigViewportsNoAutoMerge;
        configViewportsNoTaskBarIcon = io.ConfigViewportsNoTaskBarIcon;
        configViewportsNoDecoration = io.ConfigViewportsNoDecoration;
        configViewportsNoDefaultParent = io.ConfigViewportsNoDefaultParent;
        configMacOSXBehaviors = io.ConfigMacOSXBehaviors;
        configInputTextCursorBlink = io.ConfigInputTextCursorBlink;
        configInputTextEnterKeepActive = io.ConfigInputTextEnterKeepActive;
    }

    void ImGuiRenderSettingsComponent::LoadDefaults() {
        configWindowsResizeFromEdges = true;
        configWindowsMoveFromTitleBarOnly = false;
        configDragClickToInputText = false;
        configNavEnableKeyboard = true;
        configNavEnableGamepad = false;
        configNavMoveSetMousePos = false;
        configNavCaptureKeyboard = true;
        configNavEscapeClearFocusItem = true;
        configMemoryCompactTimer = -1.0f;
        configDebugHighlightIdConflicts = false;
        configDockingEnable = true;
        configDockingWithShift = false;
        configDockingAlwaysTabBar = false;
        configDockingTransparentPayload = false;
        configViewportsEnable = false;
        configViewportsNoAutoMerge = false;
        configViewportsNoTaskBarIcon = false;
        configViewportsNoDecoration = false;
        configViewportsNoDefaultParent = false;
        configMacOSXBehaviors = false;
        configInputTextCursorBlink = true;
        configInputTextEnterKeepActive = false;
    }

    void ImGuiRenderSettingsComponent::SerializeSettings(nlohmann::json& j) const {
        j["configWindowsResizeFromEdges"] = configWindowsResizeFromEdges;
        j["configWindowsMoveFromTitleBarOnly"] = configWindowsMoveFromTitleBarOnly;
        j["configDragClickToInputText"] = configDragClickToInputText;
        j["configNavEnableKeyboard"] = configNavEnableKeyboard;
        j["configNavEnableGamepad"] = configNavEnableGamepad;
        j["configNavMoveSetMousePos"] = configNavMoveSetMousePos;
        j["configNavCaptureKeyboard"] = configNavCaptureKeyboard;
        j["configNavEscapeClearFocusItem"] = configNavEscapeClearFocusItem;
        j["configMemoryCompactTimer"] = configMemoryCompactTimer;
        j["configDebugHighlightIdConflicts"] = configDebugHighlightIdConflicts;
        j["configDockingEnable"] = configDockingEnable;
        j["configDockingWithShift"] = configDockingWithShift;
        j["configDockingAlwaysTabBar"] = configDockingAlwaysTabBar;
        j["configDockingTransparentPayload"] = configDockingTransparentPayload;
        j["configViewportsEnable"] = configViewportsEnable;
        j["configViewportsNoAutoMerge"] = configViewportsNoAutoMerge;
        j["configViewportsNoTaskBarIcon"] = configViewportsNoTaskBarIcon;
        j["configViewportsNoDecoration"] = configViewportsNoDecoration;
        j["configViewportsNoDefaultParent"] = configViewportsNoDefaultParent;
        j["configMacOSXBehaviors"] = configMacOSXBehaviors;
        j["configInputTextCursorBlink"] = configInputTextCursorBlink;
        j["configInputTextEnterKeepActive"] = configInputTextEnterKeepActive;
    }

    void ImGuiRenderSettingsComponent::DeserializeSettings(const nlohmann::json& j) {
        if (j.contains("configWindowsResizeFromEdges")) configWindowsResizeFromEdges = j["configWindowsResizeFromEdges"];
        if (j.contains("configWindowsMoveFromTitleBarOnly")) configWindowsMoveFromTitleBarOnly = j["configWindowsMoveFromTitleBarOnly"];
        if (j.contains("configDragClickToInputText")) configDragClickToInputText = j["configDragClickToInputText"];
        if (j.contains("configNavEnableKeyboard")) configNavEnableKeyboard = j["configNavEnableKeyboard"];
        if (j.contains("configNavEnableGamepad")) configNavEnableGamepad = j["configNavEnableGamepad"];
        if (j.contains("configNavMoveSetMousePos")) configNavMoveSetMousePos = j["configNavMoveSetMousePos"];
        if (j.contains("configNavCaptureKeyboard")) configNavCaptureKeyboard = j["configNavCaptureKeyboard"];
        if (j.contains("configNavEscapeClearFocusItem")) configNavEscapeClearFocusItem = j["configNavEscapeClearFocusItem"];
        if (j.contains("configMemoryCompactTimer")) configMemoryCompactTimer = j["configMemoryCompactTimer"];
        if (j.contains("configDebugHighlightIdConflicts")) configDebugHighlightIdConflicts = j["configDebugHighlightIdConflicts"];
        if (j.contains("configDockingEnable")) configDockingEnable = j["configDockingEnable"];
        if (j.contains("configDockingWithShift")) configDockingWithShift = j["configDockingWithShift"];
        if (j.contains("configDockingAlwaysTabBar")) configDockingAlwaysTabBar = j["configDockingAlwaysTabBar"];
        if (j.contains("configDockingTransparentPayload")) configDockingTransparentPayload = j["configDockingTransparentPayload"];
        if (j.contains("configViewportsEnable")) configViewportsEnable = j["configViewportsEnable"];
        if (j.contains("configViewportsNoAutoMerge")) configViewportsNoAutoMerge = j["configViewportsNoAutoMerge"];
        if (j.contains("configViewportsNoTaskBarIcon")) configViewportsNoTaskBarIcon = j["configViewportsNoTaskBarIcon"];
        if (j.contains("configViewportsNoDecoration")) configViewportsNoDecoration = j["configViewportsNoDecoration"];
        if (j.contains("configViewportsNoDefaultParent")) configViewportsNoDefaultParent = j["configViewportsNoDefaultParent"];
        if (j.contains("configMacOSXBehaviors")) configMacOSXBehaviors = j["configMacOSXBehaviors"];
        if (j.contains("configInputTextCursorBlink")) configInputTextCursorBlink = j["configInputTextCursorBlink"];
        if (j.contains("configInputTextEnterKeepActive")) configInputTextEnterKeepActive = j["configInputTextEnterKeepActive"];
    }

    void ImGuiRenderSettingsComponent::ApplyAllSettingsToImGui() {
        ApplyWindowBehaviorToImGui();
        ApplyNavigationToImGui();
        ApplyDockingToImGui();
        ApplyViewportsToImGui();
    }

    void ImGuiRenderSettingsComponent::ApplyWindowBehaviorToImGui() {
        if (!imguiContext) return;
        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigWindowsResizeFromEdges = configWindowsResizeFromEdges;
        io.ConfigWindowsMoveFromTitleBarOnly = configWindowsMoveFromTitleBarOnly;
        io.ConfigDragClickToInputText = configDragClickToInputText;
    }

    void ImGuiRenderSettingsComponent::ApplyNavigationToImGui() {
        if (!imguiContext) return;
        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigNavMoveSetMousePos = configNavMoveSetMousePos;
        io.ConfigNavCaptureKeyboard = configNavCaptureKeyboard;
        io.ConfigNavEscapeClearFocusItem = configNavEscapeClearFocusItem;
        if (configNavEnableKeyboard) io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        else io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        if (configNavEnableGamepad) io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        else io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }

    void ImGuiRenderSettingsComponent::ApplyDockingToImGui() {
        if (!imguiContext) return;
        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigDockingWithShift = configDockingWithShift;
        io.ConfigDockingAlwaysTabBar = configDockingAlwaysTabBar;
        io.ConfigDockingTransparentPayload = configDockingTransparentPayload;
        if (configDockingEnable) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        else io.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
    }

    void ImGuiRenderSettingsComponent::ApplyViewportsToImGui() {
        if (!imguiContext) return;
        ImGui::SetCurrentContext(imguiContext);
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigViewportsNoAutoMerge = configViewportsNoAutoMerge;
        io.ConfigViewportsNoTaskBarIcon = configViewportsNoTaskBarIcon;
        io.ConfigViewportsNoDecoration = configViewportsNoDecoration;
        io.ConfigViewportsNoDefaultParent = configViewportsNoDefaultParent;
        if (configViewportsEnable) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        else io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }

    void ImGuiRenderSettingsComponent::RenderActionButtons() {
        if (ImGui::Button("Apply Settings")) {
            ApplyAllSettingsToImGui();
            SaveSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (hasChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
    }

} // namespace ECS