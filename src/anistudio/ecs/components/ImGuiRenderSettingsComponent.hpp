#pragma once
#include "BaseSettingsComponent.hpp"
#include <imgui.h>

namespace ECS {

    class ImGuiRenderSettingsComponent : public BaseSettingsComponent {
    public:
        ImGuiRenderSettingsComponent();

        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }
        void SetImGuiContext(ImGuiContext* context) { imguiContext = context; }

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

        bool hasChanges = false;
        ImGuiContext* imguiContext = nullptr;

        void EnsureInitialized();
        void LoadCurrentImGuiSettings();
        void ApplyAllSettingsToImGui();
        void ApplyWindowBehaviorToImGui();
        void ApplyNavigationToImGui();
        void ApplyDockingToImGui();
        void ApplyViewportsToImGui();

    private:
        ImGuiConfigFlags backupConfigFlags = ImGuiConfigFlags_None;
        bool isInitialized = false;

        void LoadDefaults();
        void SerializeSettings(nlohmann::json& j) const;
        void DeserializeSettings(const nlohmann::json& j);
    };

}