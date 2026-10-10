#include "ImGuiRenderSettingsComponent.hpp"

namespace ECS {

    nlohmann::json ImGuiRenderSettingsComponent::Serialize() const {
        nlohmann::json j;
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
        return j;
    }

    void ImGuiRenderSettingsComponent::Deserialize(const nlohmann::json& j) {
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

    void ImGuiRenderSettingsComponent::ResetToDefaults() {
        *this = ImGuiRenderSettingsComponent{};
    }

}