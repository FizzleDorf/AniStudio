#include "GeneralSettingsComponent.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace ECS {

    GeneralSettingsComponent::GeneralSettingsComponent() {
        compName = "GeneralSettingsComponent";
        LoadSettings();
        CreateBackup();
    }

    bool GeneralSettingsComponent::FilterPass(const std::string& sectionName, const std::string& filter) const {
        if (filter.empty()) return true;
        std::string lower = sectionName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string f = filter;
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return lower.find(f) != std::string::npos;
    }

    void GeneralSettingsComponent::RenderUI() {
        RenderFilteredUI("");
    }

    void GeneralSettingsComponent::RenderFilteredUI(const std::string& filter) {
        if (ImGui::BeginChild("GeneralSettings", ImVec2(0, 0), false)) {
            if (FilterPass("Startup", filter)) RenderStartupSettings();
            if (FilterPass("Auto-Save", filter)) RenderAutoSaveSettings();
            if (FilterPass("Confirmation", filter)) RenderConfirmationSettings();
            if (FilterPass("Performance", filter)) RenderPerformanceSettings();
            if (FilterPass("Logging", filter)) RenderLoggingSettings();
            RenderActionButtons();
        }
        ImGui::EndChild();
    }

    void GeneralSettingsComponent::RenderStartupSettings() {
        if (ImGui::Checkbox("Show Startup Screen", &showStartupScreen)) hasChanges = true;
        if (ImGui::Checkbox("Load Last Project on Startup", &loadLastProject)) hasChanges = true;
        ImGui::Separator();
    }

    void GeneralSettingsComponent::RenderAutoSaveSettings() {
        if (ImGui::Checkbox("Auto-Save Projects", &autoSaveProjects)) hasChanges = true;
        if (autoSaveProjects) {
            if (ImGui::SliderInt("Auto-Save Interval (minutes)", &autoSaveIntervalMinutes, 1, 60)) hasChanges = true;
        }
        ImGui::Separator();
    }

    void GeneralSettingsComponent::RenderConfirmationSettings() {
        if (ImGui::Checkbox("Confirm Before Exit", &confirmBeforeExit)) hasChanges = true;
        if (ImGui::Checkbox("Confirm Before Delete Assets", &confirmBeforeDeleteAssets)) hasChanges = true;
        if (ImGui::Checkbox("Confirm Before Overwrite Files", &confirmBeforeOverwriteFiles)) hasChanges = true;
        ImGui::Separator();
    }

    void GeneralSettingsComponent::RenderPerformanceSettings() {
        if (ImGui::SliderInt("Max Recent Projects", &maxRecentProjects, 5, 50)) hasChanges = true;
        if (ImGui::SliderInt("Max Undo Levels", &maxUndoLevels, 10, 1000)) hasChanges = true;
        if (ImGui::Checkbox("Enable Hardware Acceleration", &enableHardwareAcceleration)) hasChanges = true;
        ImGui::Separator();
    }

    void GeneralSettingsComponent::RenderLoggingSettings() {
        if (ImGui::Checkbox("Enable Logging", &enableLogging)) hasChanges = true;
        if (enableLogging) {
            const char* logLevels[] = { "Error", "Warning", "Info", "Debug" };
            if (ImGui::Combo("Log Level", &logLevel, logLevels, 4)) hasChanges = true;
            if (ImGui::Checkbox("Log to File", &logToFile)) hasChanges = true;
            if (logToFile) {
                if (ImGui::SliderInt("Max Log File Size (MB)", &maxLogFileSize, 1, 100)) hasChanges = true;
            }
        }
        ImGui::Separator();
    }

    void GeneralSettingsComponent::RenderActionButtons() {
        if (ImGui::Button("Save Settings")) SaveSettings();
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) ResetToDefaults();
        ImGui::SameLine();
        if (ImGui::Button("Revert Changes")) RestoreFromBackup();
        if (hasChanges) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
        }
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
        hasChanges = false;
    }

} // namespace ECS