#include "GeneralSettingsComponent.hpp"

namespace ECS {

    nlohmann::json GeneralSettingsComponent::Serialize() const {
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
        j["useNewestPluginVersions"] = useNewestPluginVersions;
        return j;
    }

    void GeneralSettingsComponent::Deserialize(const nlohmann::json& j) {
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
        if (j.contains("useNewestPluginVersions")) useNewestPluginVersions = j["useNewestPluginVersions"];
    }

    void GeneralSettingsComponent::ResetToDefaults() {
        *this = GeneralSettingsComponent{};
    }

}