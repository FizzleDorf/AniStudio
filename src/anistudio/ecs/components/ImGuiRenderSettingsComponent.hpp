#pragma once
#include "BaseSettingsComponent.hpp"
#include <imgui.h>

namespace ECS {

    class ImGuiRenderSettingsComponent : public BaseSettingsComponent {
    public:
        ImGuiRenderSettingsComponent();

        std::string GetTabName() const override { return "ImGui Render"; }
        std::string GetTabCategory() const override { return "Interface"; }
        void RenderUI() override;
        void RenderFilteredUI(const std::string& filter) override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }
        void SetImGuiContext(ImGuiContext* context) override { imguiContext = context; }

    private:
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

        ImGuiConfigFlags backupConfigFlags = ImGuiConfigFlags_None;
        bool hasChanges = false;
        bool isInitialized = false;
        ImGuiContext* imguiContext = nullptr;

        void EnsureInitialized();
        void LoadCurrentImGuiSettings();
        void LoadDefaults();
        void SerializeSettings(nlohmann::json& j) const;
        void DeserializeSettings(const nlohmann::json& j);
        void ApplyAllSettingsToImGui();
        void ApplyWindowBehaviorToImGui();
        void ApplyNavigationToImGui();
        void ApplyDockingToImGui();
        void ApplyViewportsToImGui();
        void RenderActionButtons();
        bool FilterPass(const std::string& section, const std::string& filter) const;
    };

} // namespace ECS