#pragma once

#include "BaseTabObject.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Settings {

	class GeneralSettingsTab : public BaseTabObject {
	public:
		GeneralSettingsTab() : BaseTabObject("General", "Application") {
			LoadSettings();
			CreateBackup();
		}

		void RenderUI() override {
			if (ImGui::BeginChild("GeneralSettings", ImVec2(0, 0), false)) {
				RenderStartupSettings();
				ImGui::Separator();
				RenderAutoSaveSettings();
				ImGui::Separator();
				RenderConfirmationSettings();
				ImGui::Separator();
				RenderPerformanceSettings();
				ImGui::Separator();
				RenderLoggingSettings();
				ImGui::Separator();
				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		void RenderFilteredUI(const std::set<std::string>& selectedCategories) override {
			if (ImGui::BeginChild("GeneralSettings", ImVec2(0, 0), false)) {
				if (ShouldRenderCategory("Startup Settings", selectedCategories)) {
					RenderStartupSettings();
					ImGui::Separator();
				}
				if (ShouldRenderCategory("Auto-Save Settings", selectedCategories)) {
					RenderAutoSaveSettings();
					ImGui::Separator();
				}
				if (ShouldRenderCategory("Confirmation Dialogs", selectedCategories)) {
					RenderConfirmationSettings();
					ImGui::Separator();
				}
				if (ShouldRenderCategory("Performance Settings", selectedCategories)) {
					RenderPerformanceSettings();
					ImGui::Separator();
				}
				if (ShouldRenderCategory("Logging Settings", selectedCategories)) {
					RenderLoggingSettings();
					ImGui::Separator();
				}
				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		bool SaveSettings() override {
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
				std::cerr << "[GeneralSettingsTab] Save error: " << e.what() << std::endl;
				return false;
			}
		}

		bool LoadSettings() override {
			try {
				std::string filePath = GetSettingsDirectory() + "/general_settings.json";

				// Try settings file first, then defaults
				if (!std::filesystem::exists(filePath)) {
					filePath = GetDefaultsDirectory() + "/general_defaults.json";
				}

				if (!std::filesystem::exists(filePath)) {
					return true; // Use hardcoded defaults
				}

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
				std::cerr << "[GeneralSettingsTab] Load error: " << e.what() << std::endl;
				return false;
			}
		}

		void ResetToDefaults() override {
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

		void CreateBackup() override {
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

		void RestoreFromBackup() override {
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

		bool HasUnsavedChanges() const override {
			return hasChanges;
		}

	private:
		// Settings data
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

		// Backup data
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

		// State tracking
		bool hasChanges = false;

		void RenderStartupSettings() {
			ImGui::Text("Startup Settings");
			ImGui::Spacing();

			if (ImGui::Checkbox("Show Startup Screen", &showStartupScreen)) hasChanges = true;
			if (ImGui::Checkbox("Load Last Project on Startup", &loadLastProject)) hasChanges = true;
		}

		void RenderAutoSaveSettings() {
			ImGui::Text("Auto-Save Settings");
			ImGui::Spacing();

			if (ImGui::Checkbox("Auto-Save Projects", &autoSaveProjects)) hasChanges = true;
			if (autoSaveProjects) {
				if (ImGui::SliderInt("Auto-Save Interval (minutes)", &autoSaveIntervalMinutes, 1, 60)) hasChanges = true;
			}
		}

		void RenderConfirmationSettings() {
			ImGui::Text("Confirmation Dialogs");
			ImGui::Spacing();

			if (ImGui::Checkbox("Confirm Before Exit", &confirmBeforeExit)) hasChanges = true;
			if (ImGui::Checkbox("Confirm Before Delete Assets", &confirmBeforeDeleteAssets)) hasChanges = true;
			if (ImGui::Checkbox("Confirm Before Overwrite Files", &confirmBeforeOverwriteFiles)) hasChanges = true;
		}

		void RenderPerformanceSettings() {
			ImGui::Text("Performance Settings");
			ImGui::Spacing();

			if (ImGui::SliderInt("Max Recent Projects", &maxRecentProjects, 5, 50)) hasChanges = true;
			if (ImGui::SliderInt("Max Undo Levels", &maxUndoLevels, 10, 1000)) hasChanges = true;
			if (ImGui::Checkbox("Enable Hardware Acceleration", &enableHardwareAcceleration)) hasChanges = true;
		}

		void RenderLoggingSettings() {
			ImGui::Text("Logging Settings");
			ImGui::Spacing();

			if (ImGui::Checkbox("Enable Logging", &enableLogging)) hasChanges = true;
			if (enableLogging) {
				const char* logLevels[] = { "Error", "Warning", "Info", "Debug" };
				if (ImGui::Combo("Log Level", &logLevel, logLevels, 4)) hasChanges = true;
				if (ImGui::Checkbox("Log to File", &logToFile)) hasChanges = true;
				if (logToFile) {
					if (ImGui::SliderInt("Max Log File Size (MB)", &maxLogFileSize, 1, 100)) hasChanges = true;
				}
			}
		}

		void RenderActionButtons() {
			if (ImGui::Button("Save Settings")) {
				SaveSettings();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset to Defaults")) {
				ResetToDefaults();
			}
			ImGui::SameLine();
			if (ImGui::Button("Revert Changes")) {
				RestoreFromBackup();
			}

			if (hasChanges) {
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Unsaved changes");
			}
		}
	};

} // namespace Settings