#pragma once
#include "BaseSettingsComponent.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace ECS {

    class GeneralSettingsComponent : public BaseSettingsComponent {
    public:
        GeneralSettingsComponent();

        bool SaveSettings() override;
        bool LoadSettings() override;
        void ResetToDefaults() override;
        void CreateBackup() override;
        void RestoreFromBackup() override;
        bool HasUnsavedChanges() const override { return hasChanges; }

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
        bool hasChanges = false;

        int windowWidth = 1200;
        int windowHeight = 720;
        int windowPosX = -1;
        int windowPosY = -1;
        bool windowMaximized = false;
        bool windowFullscreen = false;
        bool windowVsync = true;
        std::string lastOpenProject;
        std::vector<std::string> recentProjects;

    private:
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

        int backupWindowWidth = 1200;
        int backupWindowHeight = 720;
        int backupWindowPosX = -1;
        int backupWindowPosY = -1;
        bool backupWindowMaximized = false;
        bool backupWindowFullscreen = false;
        bool backupWindowVsync = true;
        std::string backupLastOpenProject;
        std::vector<std::string> backupRecentProjects;
    };

}