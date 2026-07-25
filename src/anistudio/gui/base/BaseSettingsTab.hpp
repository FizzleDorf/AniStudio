#pragma once
#include <string>
#include <imgui.h>

namespace ECS {

    class BaseSettingsTab {
    public:
        virtual ~BaseSettingsTab() = default;

        virtual std::string GetTabName() const = 0;
        virtual std::string GetTabCategory() const = 0;
        virtual void Render() = 0;
        virtual void SetFilter(const std::string& filter) = 0;

        virtual bool HasUnsavedChanges() const = 0;
        virtual void CreateBackup() = 0;
        virtual void RestoreFromBackup() = 0;
        virtual void ResetToDefaults() = 0;
        virtual bool SaveSettings() = 0;
        virtual bool LoadSettings() = 0;
        virtual void SetImGuiContext(ImGuiContext* ctx) = 0;
    };

}