#include "ImGuiRenderSettingsComponent.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>

namespace ECS {

    ImGuiRenderSettingsComponent::ImGuiRenderSettingsComponent() {
        compName = "ImGuiRenderSettingsComponent";
    }

    void ImGuiRenderSettingsComponent::EnsureInitialized() {
        if (!isInitialized && imguiContext) {
            ImGui::SetCurrentContext(imguiContext);
            LoadCurrentImGuiSettings();
            isInitialized = true;
        }
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

}