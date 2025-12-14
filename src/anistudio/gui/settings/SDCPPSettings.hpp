#pragma once

#include "BaseTabObject.hpp"
#include "ImGuiFileDialog.h"
#include "FilePaths.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Settings {

	class SDCPPSettingsTab : public BaseTabObject {
	public:
		SDCPPSettingsTab() : BaseTabObject("SDCPP", "Models") {
			LoadFromFilePaths();
			LoadSettings();
			CreateBackup();
		}

		void RenderUI() override {
			if (ImGui::BeginChild("SDCPPSettings", ImVec2(0, 0), false)) {
				// SDCPP Configuration
				ImGui::Text("SDCPP Configuration");
				ImGui::Spacing();

				if (ImGui::BeginTable("SDCPPTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Setting Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					RenderPathRow("SDCPP Models Root", sdcppModelsRoot);
					RenderPathRow("SDCPP Config Path", sdcppConfigPath);

					ImGui::EndTable();
				}

				ImGui::Separator();

				// SDCPP Controls
				ImGui::Text("SDCPP Controls");
				ImGui::Spacing();

				if (ImGui::Button("Initialize SDCPP")) {
					// SDCPP initialization logic here
					hasChanges = true;
				}

				ImGui::SameLine();
				if (ImGui::Button("Test Connection")) {
					// Test SDCPP connection logic here
				}

				ImGui::Separator();
				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		void RenderFilteredUI(const std::set<std::string>& selectedCategories) override {
			if (ImGui::BeginChild("SDCPPSettings", ImVec2(0, 0), false)) {
				// SDCPP Configuration
				if (ShouldRenderCategory("SDCPP Configuration", selectedCategories)) {
					ImGui::Text("SDCPP Configuration");
					ImGui::Spacing();

					if (ImGui::BeginTable("SDCPPTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("Setting Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
						ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
						ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();

						RenderPathRow("SDCPP Models Root", sdcppModelsRoot);
						RenderPathRow("SDCPP Config Path", sdcppConfigPath);

						ImGui::EndTable();
					}
					ImGui::Separator();
				}

				// SDCPP Controls
				if (ShouldRenderCategory("SDCPP Controls", selectedCategories)) {
					ImGui::Text("SDCPP Controls");
					ImGui::Spacing();

					if (ImGui::Button("Initialize SDCPP")) {
						// SDCPP initialization logic here
						hasChanges = true;
					}

					ImGui::SameLine();
					if (ImGui::Button("Test Connection")) {
						// Test SDCPP connection logic here
					}
					ImGui::Separator();
				}

				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		bool SaveSettings() override {
			try {
				nlohmann::json j;
				j["sdcppModelsRoot"] = sdcppModelsRoot;
				j["sdcppConfigPath"] = sdcppConfigPath;

				std::string filePath = GetSettingsDirectory() + "/sdcpp_settings.json";
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
				std::cerr << "[SDCPPSettingsTab] Save error: " << e.what() << std::endl;
				return false;
			}
		}

		bool LoadSettings() override {
			try {
				std::string filePath = GetSettingsDirectory() + "/sdcpp_settings.json";

				// Try settings file first, then defaults
				if (!std::filesystem::exists(filePath)) {
					filePath = GetDefaultsDirectory() + "/sdcpp_defaults.json";
				}

				if (!std::filesystem::exists(filePath)) {
					LoadFromFilePaths();
					return true;
				}

				std::ifstream file(filePath);
				if (!file.is_open()) return false;

				nlohmann::json j;
				file >> j;
				file.close();

				if (j.contains("sdcppModelsRoot")) sdcppModelsRoot = j["sdcppModelsRoot"];
				if (j.contains("sdcppConfigPath")) sdcppConfigPath = j["sdcppConfigPath"];

				hasChanges = false;
				CreateBackup();
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[SDCPPSettingsTab] Load error: " << e.what() << std::endl;
				return false;
			}
		}

		void ResetToDefaults() override {
			sdcppModelsRoot.clear();
			sdcppConfigPath.clear();
			hasChanges = true;
		}

		void CreateBackup() override {
			backupSdcppModelsRoot = sdcppModelsRoot;
			backupSdcppConfigPath = sdcppConfigPath;
		}

		void RestoreFromBackup() override {
			sdcppModelsRoot = backupSdcppModelsRoot;
			sdcppConfigPath = backupSdcppConfigPath;
			hasChanges = false;
		}

		bool HasUnsavedChanges() const override {
			return hasChanges;
		}

	private:
		// SDCPP-specific settings data
		std::string sdcppModelsRoot;
		std::string sdcppConfigPath;

		// Backup data
		std::string backupSdcppModelsRoot;
		std::string backupSdcppConfigPath;

		// State tracking
		bool hasChanges = false;

		void LoadFromFilePaths() {
			// Load SDCPP-specific paths if available
			sdcppModelsRoot = Utils::FilePaths::GetInstance().GetPath("ModelRoot");
			sdcppConfigPath = "data/sdcpp/config.json";
		}

		void RenderPathRow(const char* label, std::string& path) {
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(label);

			ImGui::TableNextColumn();
			std::string buttonID = std::string("...##") + label;
			if (ImGui::Button(buttonID.c_str())) {
				IGFD::FileDialogConfig config;
				config.path = path.empty() ? "." : path;
				config.flags = ImGuiFileDialogFlags_Modal;
				std::string dialogID = std::string("ChoosePath##") + label;
				ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(), "Select Directory", nullptr, config);
			}

			std::string dialogID = std::string("ChoosePath##") + label;
			if (ImGuiFileDialog::Instance()->Display(dialogID.c_str(), 32, ImVec2(500, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
					if (!selectedPath.empty() && selectedPath != path) {
						path = selectedPath;
						hasChanges = true;
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();
			std::string resetID = std::string("R##") + label;
			if (ImGui::Button(resetID.c_str())) {
				if (!path.empty()) {
					path.clear();
					hasChanges = true;
				}
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", path.c_str());
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