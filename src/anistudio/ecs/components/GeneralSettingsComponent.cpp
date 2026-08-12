#include "GeneralSettingsComponent.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace ECS {

    GeneralSettingsComponent::GeneralSettingsComponent() {
        compName = "GeneralSettingsComponent";
        LoadSettings();
        CreateBackup();
    }

    bool GeneralSettingsComponent::SaveSettings() {
        try {
            nlohmann::json j;
            j["showStartupScreen"] = showStartupScreen;
            j["loadLastProject"] = loadLastProject;
            j["autoSaveProjects"] = autoSaveProjects;
            j["autoSaveIntervalMinutes"] = autoSaveIntervalMinutes;
            j["confirmBeforeExit"] = confirmBeforeExit;
            j["confirmBeforeDeleteAssets"] = confirmBeforeDeleteAssets;
            j["confirmBeforeOverwriteFiles"] = confirmBeforeOverwriteFiles;
            j["maxRecentProjects"] = maxRecentProjects;
            j["maxUndoLevels"] = maxUndoLevels;
            j["enableHardwareAcceleration"] = enableHardwareAcceleration;
            j["enableLogging"] = enableLogging;
            j["logLevel"] = logLevel;
            j["logToFile"] = logToFile;
            j["maxLogFileSize"] = maxLogFileSize;
            j["windowWidth"] = windowWidth;
            j["windowHeight"] = windowHeight;
            j["windowPosX"] = windowPosX;
            j["windowPosY"] = windowPosY;
            j["windowMaximized"] = windowMaximized;
            j["windowFullscreen"] = windowFullscreen;
            j["windowVsync"] = windowVsync;
            j["lastOpenProject"] = lastOpenProject;
            j["recentProjects"] = recentProjects;

            std::string filePath = GetSettingsDirectory() + "/general_settings.json";
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
            std::cerr << "[GeneralSettingsComponent] Save error: " << e.what() << std::endl;
            return false;
        }
    }

    bool GeneralSettingsComponent::LoadSettings() {
        try {
            std::string filePath = GetSettingsDirectory() + "/general_settings.json";
            if (!std::filesystem::exists(filePath)) {
                filePath = GetDefaultsDirectory() + "/general_defaults.json";
            }
            if (!std::filesystem::exists(filePath)) return true;

            std::ifstream file(filePath);
            if (!file.is_open()) return false;
            nlohmann::json j;
            file >> j;
            file.close();

            if (j.contains("showStartupScreen")) showStartupScreen = j["showStartupScreen"];
            if (j.contains("loadLastProject")) loadLastProject = j["loadLastProject"];
            if (j.contains("autoSaveProjects")) autoSaveProjects = j["autoSaveProjects"];
            if (j.contains("autoSaveIntervalMinutes")) autoSaveIntervalMinutes = j["autoSaveIntervalMinutes"];
            if (j.contains("confirmBeforeExit")) confirmBeforeExit = j["confirmBeforeExit"];
            if (j.contains("confirmBeforeDeleteAssets")) confirmBeforeDeleteAssets = j["confirmBeforeDeleteAssets"];
            if (j.contains("confirmBeforeOverwriteFiles")) confirmBeforeOverwriteFiles = j["confirmBeforeOverwriteFiles"];
            if (j.contains("maxRecentProjects")) maxRecentProjects = j["maxRecentProjects"];
            if (j.contains("maxUndoLevels")) maxUndoLevels = j["maxUndoLevels"];
            if (j.contains("enableHardwareAcceleration")) enableHardwareAcceleration = j["enableHardwareAcceleration"];
            if (j.contains("enableLogging")) enableLogging = j["enableLogging"];
            if (j.contains("logLevel")) logLevel = j["logLevel"];
            if (j.contains("logToFile")) logToFile = j["logToFile"];
            if (j.contains("maxLogFileSize")) maxLogFileSize = j["maxLogFileSize"];
            if (j.contains("windowWidth")) windowWidth = j["windowWidth"];
            if (j.contains("windowHeight")) windowHeight = j["windowHeight"];
            if (j.contains("windowPosX")) windowPosX = j["windowPosX"];
            if (j.contains("windowPosY")) windowPosY = j["windowPosY"];
            if (j.contains("windowMaximized")) windowMaximized = j["windowMaximized"];
            if (j.contains("windowFullscreen")) windowFullscreen = j["windowFullscreen"];
            if (j.contains("windowVsync")) windowVsync = j["windowVsync"];
            if (j.contains("lastOpenProject")) lastOpenProject = j["lastOpenProject"];
            if (j.contains("recentProjects")) recentProjects = j["recentProjects"];

            hasChanges = false;
            CreateBackup();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "[GeneralSettingsComponent] Load error: " << e.what() << std::endl;
            return false;
        }
    }

    void GeneralSettingsComponent::ResetToDefaults() {
        showStartupScreen = true;
        loadLastProject = true;
        autoSaveProjects = true;
        autoSaveIntervalMinutes = 5;
        confirmBeforeExit = true;
        confirmBeforeDeleteAssets = true;
        confirmBeforeOverwriteFiles = true;
        maxRecentProjects = 10;
        maxUndoLevels = 100;
        enableHardwareAcceleration = true;
        enableLogging = true;
        logLevel = 2;
        logToFile = true;
        maxLogFileSize = 10;
        windowWidth = 1200;
        windowHeight = 720;
        windowPosX = -1;
        windowPosY = -1;
        windowMaximized = false;
        windowFullscreen = false;
        windowVsync = true;
        lastOpenProject.clear();
        recentProjects.clear();
        hasChanges = true;
    }

    void GeneralSettingsComponent::CreateBackup() {
        backupShowStartupScreen = showStartupScreen;
        backupLoadLastProject = loadLastProject;
        backupAutoSaveProjects = autoSaveProjects;
        backupAutoSaveIntervalMinutes = autoSaveIntervalMinutes;
        backupConfirmBeforeExit = confirmBeforeExit;
        backupConfirmBeforeDeleteAssets = confirmBeforeDeleteAssets;
        backupConfirmBeforeOverwriteFiles = confirmBeforeOverwriteFiles;
        backupMaxRecentProjects = maxRecentProjects;
        backupMaxUndoLevels = maxUndoLevels;
        backupEnableHardwareAcceleration = enableHardwareAcceleration;
        backupEnableLogging = enableLogging;
        backupLogLevel = logLevel;
        backupLogToFile = logToFile;
        backupMaxLogFileSize = maxLogFileSize;
        backupWindowWidth = windowWidth;
        backupWindowHeight = windowHeight;
        backupWindowPosX = windowPosX;
        backupWindowPosY = windowPosY;
        backupWindowMaximized = windowMaximized;
        backupWindowFullscreen = windowFullscreen;
        backupWindowVsync = windowVsync;
        backupLastOpenProject = lastOpenProject;
        backupRecentProjects = recentProjects;
    }

    void GeneralSettingsComponent::RestoreFromBackup() {
        showStartupScreen = backupShowStartupScreen;
        loadLastProject = backupLoadLastProject;
        autoSaveProjects = backupAutoSaveProjects;
        autoSaveIntervalMinutes = backupAutoSaveIntervalMinutes;
        confirmBeforeExit = backupConfirmBeforeExit;
        confirmBeforeDeleteAssets = backupConfirmBeforeDeleteAssets;
        confirmBeforeOverwriteFiles = backupConfirmBeforeOverwriteFiles;
        maxRecentProjects = backupMaxRecentProjects;
        maxUndoLevels = backupMaxUndoLevels;
        enableHardwareAcceleration = backupEnableHardwareAcceleration;
        enableLogging = backupEnableLogging;
        logLevel = backupLogLevel;
        logToFile = backupLogToFile;
        maxLogFileSize = backupMaxLogFileSize;
        windowWidth = backupWindowWidth;
        windowHeight = backupWindowHeight;
        windowPosX = backupWindowPosX;
        windowPosY = backupWindowPosY;
        windowMaximized = backupWindowMaximized;
        windowFullscreen = backupWindowFullscreen;
        windowVsync = backupWindowVsync;
        lastOpenProject = backupLastOpenProject;
        recentProjects = backupRecentProjects;
        hasChanges = false;
    }

}