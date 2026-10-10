#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <nlohmann/json.hpp>
#include <imgui.h>

namespace ECS {

    class ImGuiRenderSettingsComponent : public BaseComponent {
    public:
        ImGuiRenderSettingsComponent() = default;

        const char* GetCompName() const override { return "ImGuiRenderSettingsComponent"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "ImGui Render Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "configWindowsResizeFromEdges", "configWindowsMoveFromTitleBarOnly", "configDragClickToInputText",
                    "configNavEnableKeyboard", "configNavEnableGamepad", "configNavMoveSetMousePos",
                    "configNavCaptureKeyboard", "configNavEscapeClearFocusItem",
                    "configMemoryCompactTimer", "configDebugHighlightIdConflicts",
                    "configDockingEnable", "configDockingWithShift", "configDockingAlwaysTabBar", "configDockingTransparentPayload",
                    "configViewportsEnable", "configViewportsNoAutoMerge", "configViewportsNoTaskBarIcon",
                    "configViewportsNoDecoration", "configViewportsNoDefaultParent",
                    "configMacOSXBehaviors", "configInputTextCursorBlink", "configInputTextEnterKeepActive"
                }},
                {"properties", {
                    {"configWindowsResizeFromEdges", {{"type","boolean"},{"title","Windows Resize From Edges"},{"ui:widget","checkbox"}}},
                    {"configWindowsMoveFromTitleBarOnly", {{"type","boolean"},{"title","Windows Move From Title Bar Only"},{"ui:widget","checkbox"}}},
                    {"configDragClickToInputText", {{"type","boolean"},{"title","Drag Click To Input Text"},{"ui:widget","checkbox"}}},
                    {"configNavEnableKeyboard", {{"type","boolean"},{"title","Enable Keyboard Navigation"},{"ui:widget","checkbox"}}},
                    {"configNavEnableGamepad", {{"type","boolean"},{"title","Enable Gamepad Navigation"},{"ui:widget","checkbox"}}},
                    {"configNavMoveSetMousePos", {{"type","boolean"},{"title","Nav Move Set Mouse Pos"},{"ui:widget","checkbox"}}},
                    {"configNavCaptureKeyboard", {{"type","boolean"},{"title","Nav Capture Keyboard"},{"ui:widget","checkbox"}}},
                    {"configNavEscapeClearFocusItem", {{"type","boolean"},{"title","Nav Escape Clear Focus Item"},{"ui:widget","checkbox"}}},
                    {"configMemoryCompactTimer", {{"type","number"},{"title","Memory Compact Timer"},{"description","Seconds between memory compaction; -1 disables."},{"ui:widget","input_float"}}},
                    {"configDebugHighlightIdConflicts", {{"type","boolean"},{"title","Debug Highlight ID Conflicts"},{"ui:widget","checkbox"}}},
                    {"configDockingEnable", {{"type","boolean"},{"title","Enable Docking"},{"ui:widget","checkbox"}}},
                    {"configDockingWithShift", {{"type","boolean"},{"title","Docking With Shift"},{"ui:widget","checkbox"}}},
                    {"configDockingAlwaysTabBar", {{"type","boolean"},{"title","Docking Always Tab Bar"},{"ui:widget","checkbox"}}},
                    {"configDockingTransparentPayload", {{"type","boolean"},{"title","Docking Transparent Payload"},{"ui:widget","checkbox"}}},
                    {"configViewportsEnable", {{"type","boolean"},{"title","Enable Viewports"},{"ui:widget","checkbox"}}},
                    {"configViewportsNoAutoMerge", {{"type","boolean"},{"title","Viewports No Auto Merge"},{"ui:widget","checkbox"}}},
                    {"configViewportsNoTaskBarIcon", {{"type","boolean"},{"title","Viewports No Task Bar Icon"},{"ui:widget","checkbox"}}},
                    {"configViewportsNoDecoration", {{"type","boolean"},{"title","Viewports No Decoration"},{"ui:widget","checkbox"}}},
                    {"configViewportsNoDefaultParent", {{"type","boolean"},{"title","Viewports No Default Parent"},{"ui:widget","checkbox"}}},
                    {"configMacOSXBehaviors", {{"type","boolean"},{"title","macOS X Behaviors"},{"ui:widget","checkbox"}}},
                    {"configInputTextCursorBlink", {{"type","boolean"},{"title","Input Text Cursor Blink"},{"ui:widget","checkbox"}}},
                    {"configInputTextEnterKeepActive", {{"type","boolean"},{"title","Input Text Enter Keep Active"},{"ui:widget","checkbox"}}}
                }}
            };
            return j;
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"configWindowsResizeFromEdges", &configWindowsResizeFromEdges},
                {"configWindowsMoveFromTitleBarOnly", &configWindowsMoveFromTitleBarOnly},
                {"configDragClickToInputText", &configDragClickToInputText},
                {"configNavEnableKeyboard", &configNavEnableKeyboard},
                {"configNavEnableGamepad", &configNavEnableGamepad},
                {"configNavMoveSetMousePos", &configNavMoveSetMousePos},
                {"configNavCaptureKeyboard", &configNavCaptureKeyboard},
                {"configNavEscapeClearFocusItem", &configNavEscapeClearFocusItem},
                {"configMemoryCompactTimer", &configMemoryCompactTimer},
                {"configDebugHighlightIdConflicts", &configDebugHighlightIdConflicts},
                {"configDockingEnable", &configDockingEnable},
                {"configDockingWithShift", &configDockingWithShift},
                {"configDockingAlwaysTabBar", &configDockingAlwaysTabBar},
                {"configDockingTransparentPayload", &configDockingTransparentPayload},
                {"configViewportsEnable", &configViewportsEnable},
                {"configViewportsNoAutoMerge", &configViewportsNoAutoMerge},
                {"configViewportsNoTaskBarIcon", &configViewportsNoTaskBarIcon},
                {"configViewportsNoDecoration", &configViewportsNoDecoration},
                {"configViewportsNoDefaultParent", &configViewportsNoDefaultParent},
                {"configMacOSXBehaviors", &configMacOSXBehaviors},
                {"configInputTextCursorBlink", &configInputTextCursorBlink},
                {"configInputTextEnterKeepActive", &configInputTextEnterKeepActive}
            };
        }

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void ResetToDefaults();

        bool configWindowsResizeFromEdges = true;
        bool configWindowsMoveFromTitleBarOnly = false;
        bool configDragClickToInputText = false;
        bool configNavEnableKeyboard = true;
        bool configNavEnableGamepad = false;
        bool configNavMoveSetMousePos = false;
        bool configNavCaptureKeyboard = true;
        bool configNavEscapeClearFocusItem = true;
        float configMemoryCompactTimer = -1.0f;
        bool configDebugHighlightIdConflicts = false;
        bool configDockingEnable = true;
        bool configDockingWithShift = false;
        bool configDockingAlwaysTabBar = false;
        bool configDockingTransparentPayload = false;
        bool configViewportsEnable = false;
        bool configViewportsNoAutoMerge = false;
        bool configViewportsNoTaskBarIcon = false;
        bool configViewportsNoDecoration = false;
        bool configViewportsNoDefaultParent = false;
        bool configMacOSXBehaviors = false;
        bool configInputTextCursorBlink = true;
        bool configInputTextEnterKeepActive = false;
    };

}