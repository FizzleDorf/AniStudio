#pragma once

#include "BaseTabObject.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Settings {

    class ImGuiRenderSettingsTab : public BaseTabObject {
    public:
        ImGuiRenderSettingsTab() : BaseTabObject("ImGui Render", "Interface") {
            hasChanges = false;
            isInitialized = false;
            imguiContext = nullptr;
        }

        void SetImGuiContext(ImGuiContext* context) {
            imguiContext = context;
        }

        void RenderUI() override {
            EnsureInitialized();
            RenderFilteredUI({});
        }

        void RenderFilteredUI(const std::set<std::string>& selectedCategories) override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override;

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

        bool hasChanges = false;
        bool isInitialized = false;
        ImGuiContext* imguiContext;
        ImGuiConfigFlags backupConfigFlags = ImGuiConfigFlags_None;

        void EnsureInitialized();
        void LoadCurrentImGuiSettings();
        void LoadDefaults();
        void SerializeSettings(nlohmann::json& j);
        void DeserializeSettings(const nlohmann::json& j);
        void ApplyAllSettingsToImGui();
        void ApplyWindowBehaviorToImGui();
        void ApplyNavigationToImGui();
        void ApplyDockingToImGui();
        void ApplyViewportsToImGui();
        void RenderActionButtons();
    };

} // namespace Settings