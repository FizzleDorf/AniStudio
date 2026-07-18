#pragma once
#include "BaseSettingsComponent.hpp"
#include <nlohmann/json.hpp>

namespace ECS {

    class GeneralSettingsComponent : public BaseSettingsComponent {
    public:
        GeneralSettingsComponent();

        std::string GetTabName() const override { return "General"; }
        std::string GetTabCategory() const override { return "Application"; }
        void RenderUI() override;
        void RenderFilteredUI(const std::string& filter) override;
        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }

    private:
        bool showStartupScreen = true;
        bool loadLastProject = true;
        bool autoSaveProjects = true;
        int autoSaveIntervalMinutes = 5;
        bool confirmBeforeExit = true;
        bool confirmBeforeDeleteAssets = true;
        bool confirmBeforeOverwriteFiles = true;
        int maxRecentProjects = 10;
        int maxUndoLevels = 100;
        bool enableHardwareAcceleration = true;
        bool enableLogging = true;
        int logLevel = 2;
        bool logToFile = true;
        int maxLogFileSize = 10;

        bool backupShowStartupScreen = true;
        bool backupLoadLastProject = true;
        bool backupAutoSaveProjects = true;
        int backupAutoSaveIntervalMinutes = 5;
        bool backupConfirmBeforeExit = true;
        bool backupConfirmBeforeDeleteAssets = true;
        bool backupConfirmBeforeOverwriteFiles = true;
        int backupMaxRecentProjects = 10;
        int backupMaxUndoLevels = 100;
        bool backupEnableHardwareAcceleration = true;
        bool backupEnableLogging = true;
        int backupLogLevel = 2;
        bool backupLogToFile = true;
        int backupMaxLogFileSize = 10;

        bool hasChanges = false;

        void RenderStartupSettings();
        void RenderAutoSaveSettings();
        void RenderConfirmationSettings();
        void RenderPerformanceSettings();
        void RenderLoggingSettings();
        void RenderActionButtons();
        bool FilterPass(const std::string& sectionName, const std::string& filter) const;
    };

} // namespace ECS