#pragma once
#include "BaseTabObject.hpp"
#include "GuiStyleHelpers.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <imgui.h>

namespace Settings {
    class ImGuiStyleSettingsTab : public BaseTabObject {
    public:
        ImGuiStyleSettingsTab() : BaseTabObject("ImGui Style", "Interface") {
            hasChanges = false;
            isInitialized = false;
            imguiContext = nullptr;
        }

        void SetImGuiContext(ImGuiContext* context) {
            imguiContext = context;
            if (imguiContext) {
                LoadSettings();
                CreateBackup();
            }
        }

        void RenderUI() override {
            if (!imguiContext) return;

            ImGui::SetCurrentContext(imguiContext);
            EnsureInitialized();
            RenderFilteredUI({});
        }

        void RenderFilteredUI(const std::set<std::string>& selectedCategories) override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }

    private:
        ImGuiStyle backupStyle;
        bool hasChanges = false;
        bool isInitialized = false;
        ImGuiContext* imguiContext;

        void EnsureInitialized();
        bool ShowStyleSelector(const char* label);
        void ShowFontSelector(const char* label);
        void RenderActionButtons();
        void SaveStyleToFile(const ImGuiStyle& style, const std::string& filename);
        void LoadStyleFromFile(ImGuiStyle& style, const std::string& filename);
    };
}