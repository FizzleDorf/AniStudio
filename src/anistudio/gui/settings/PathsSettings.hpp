#pragma once

#include "BaseTabObject.hpp"
#include "ImGuiFileDialog.h"
#include "FilePaths.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace Settings {

	class PathsSettingsTab : public BaseTabObject {
	public:
		PathsSettingsTab() : BaseTabObject("Paths", "Application") {
			LoadFromFilePaths();
			LoadSettings();
			CreateBackup();
		}

		void RenderUI() override {
			if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
				// General Paths Section
				ImGui::Text("General Paths");
				ImGui::Spacing();

				if (ImGui::BeginTable("GeneralPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
					ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					RenderPathRow("Last Open Project", lastOpenProjectPath);
					RenderPathRow("Default Project", defaultProjectPath);
					RenderPathRow("Assets Folder", assetsFolderPath);

					ImGui::EndTable();
				}

				ImGui::Separator();

				if (ImGui::Button("Reset Model Paths to Root")) {
					if (!defaultModelRootPath.empty()) {
						checkpointDir = defaultModelRootPath + "/checkpoints";
						encoderDir = defaultModelRootPath + "/clip";
						vaeDir = defaultModelRootPath + "/vae";
						unetDir = defaultModelRootPath + "/unet";
						loraDir = defaultModelRootPath + "/loras";
						controlnetDir = defaultModelRootPath + "/controlnet";
						upscaleDir = defaultModelRootPath + "/upscale_models";
						hasChanges = true;
					}
				}

				ImGui::Separator();

				// Model Paths Section
				ImGui::Text("Model Paths");
				ImGui::Spacing();

				if (ImGui::BeginTable("ModelPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
					ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableHeadersRow();

					RenderPathRow("Model Root", defaultModelRootPath);
					RenderPathRow("Checkpoints", checkpointDir);
					RenderPathRow("Text Encoders", encoderDir);
					RenderPathRow("VAE", vaeDir);
					RenderPathRow("UNet", unetDir);
					RenderPathRow("LORA", loraDir);
					RenderPathRow("ControlNet", controlnetDir);
					RenderPathRow("Upscale", upscaleDir);

					ImGui::EndTable();
				}

				ImGui::Separator();
				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		void RenderFilteredUI(const std::set<std::string>& selectedCategories) override {
			if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
				// General Paths Section
				if (ShouldRenderCategory("General Paths", selectedCategories)) {
					ImGui::Text("General Paths");
					ImGui::Spacing();

					if (ImGui::BeginTable("GeneralPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
						ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
						ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();

						RenderPathRow("Last Open Project", lastOpenProjectPath);
						RenderPathRow("Default Project", defaultProjectPath);
						RenderPathRow("Assets Folder", assetsFolderPath);

						ImGui::EndTable();
					}
					ImGui::Separator();
				}

				if (ImGui::Button("Reset Model Paths to Root")) {
					if (!defaultModelRootPath.empty()) {
						checkpointDir = defaultModelRootPath + "/checkpoints";
						encoderDir = defaultModelRootPath + "/text_encoders";
						vaeDir = defaultModelRootPath + "/vae";
						unetDir = defaultModelRootPath + "/unet";
						loraDir = defaultModelRootPath + "/lora";
						controlnetDir = defaultModelRootPath + "/controlnet";
						upscaleDir = defaultModelRootPath + "/upscale";
						hasChanges = true;
					}
				}

				ImGui::Separator();

				// Model Paths Section
				if (ShouldRenderCategory("Model Paths", selectedCategories)) {
					ImGui::Text("Model Paths");
					ImGui::Spacing();

					if (ImGui::BeginTable("ModelPathsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
						ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
						ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
						ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();

						RenderPathRow("Model Root", defaultModelRootPath);
						RenderPathRow("Checkpoints", checkpointDir);
						RenderPathRow("Text Encoders", encoderDir);
						RenderPathRow("VAE", vaeDir);
						RenderPathRow("UNet", unetDir);
						RenderPathRow("LORA", loraDir);
						RenderPathRow("ControlNet", controlnetDir);
						RenderPathRow("Upscale", upscaleDir);

						ImGui::EndTable();
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
				j["lastOpenProjectPath"] = lastOpenProjectPath;
				j["defaultProjectPath"] = defaultProjectPath;
				j["assetsFolderPath"] = assetsFolderPath;
				j["defaultModelRootPath"] = defaultModelRootPath;
				j["checkpointDir"] = checkpointDir;
				j["encoderDir"] = encoderDir;
				j["vaeDir"] = vaeDir;
				j["unetDir"] = unetDir;
				j["loraDir"] = loraDir;
				j["controlnetDir"] = controlnetDir;
				j["upscaleDir"] = upscaleDir;

				std::string filePath = GetSettingsDirectory() + "/paths_settings.json";
				std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

				std::ofstream file(filePath);
				if (!file.is_open()) return false;

				file << j.dump(4);
				file.close();

				// Apply to FilePaths system
				ApplyToFilePaths();

				hasChanges = false;
				CreateBackup();
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[PathsSettingsTab] Save error: " << e.what() << std::endl;
				return false;
			}
		}

		bool LoadSettings() override {
			try {
				std::string filePath = GetSettingsDirectory() + "/paths_settings.json";

				// Try settings file first, then defaults
				if (!std::filesystem::exists(filePath)) {
					filePath = GetDefaultsDirectory() + "/paths_defaults.json";
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

				if (j.contains("lastOpenProjectPath")) lastOpenProjectPath = j["lastOpenProjectPath"];
				if (j.contains("defaultProjectPath")) defaultProjectPath = j["defaultProjectPath"];
				if (j.contains("assetsFolderPath")) assetsFolderPath = j["assetsFolderPath"];
				if (j.contains("defaultModelRootPath")) defaultModelRootPath = j["defaultModelRootPath"];
				if (j.contains("checkpointDir")) checkpointDir = j["checkpointDir"];
				if (j.contains("encoderDir")) encoderDir = j["encoderDir"];
				if (j.contains("vaeDir")) vaeDir = j["vaeDir"];
				if (j.contains("unetDir")) unetDir = j["unetDir"];
				if (j.contains("loraDir")) loraDir = j["loraDir"];
				if (j.contains("controlnetDir")) controlnetDir = j["controlnetDir"];
				if (j.contains("upscaleDir")) upscaleDir = j["upscaleDir"];

				hasChanges = false;
				CreateBackup();
				return true;
			}
			catch (const std::exception& e) {
				std::cerr << "[PathsSettingsTab] Load error: " << e.what() << std::endl;
				return false;
			}
		}

		void ResetToDefaults() override {
			lastOpenProjectPath.clear();
			defaultProjectPath.clear();
			assetsFolderPath.clear();
			defaultModelRootPath.clear();
			checkpointDir.clear();
			encoderDir.clear();
			vaeDir.clear();
			unetDir.clear();
			loraDir.clear();
			controlnetDir.clear();
			upscaleDir.clear();
			hasChanges = true;
		}

		void CreateBackup() override {
			backupLastOpenProjectPath = lastOpenProjectPath;
			backupDefaultProjectPath = defaultProjectPath;
			backupAssetsFolderPath = assetsFolderPath;
			backupDefaultModelRootPath = defaultModelRootPath;
			backupCheckpointDir = checkpointDir;
			backupEncoderDir = encoderDir;
			backupVaeDir = vaeDir;
			backupUnetDir = unetDir;
			backupLoraDir = loraDir;
			backupControlnetDir = controlnetDir;
			backupUpscaleDir = upscaleDir;
		}

		void RestoreFromBackup() override {
			lastOpenProjectPath = backupLastOpenProjectPath;
			defaultProjectPath = backupDefaultProjectPath;
			assetsFolderPath = backupAssetsFolderPath;
			defaultModelRootPath = backupDefaultModelRootPath;
			checkpointDir = backupCheckpointDir;
			encoderDir = backupEncoderDir;
			vaeDir = backupVaeDir;
			unetDir = backupUnetDir;
			loraDir = backupLoraDir;
			controlnetDir = backupControlnetDir;
			upscaleDir = backupUpscaleDir;
			hasChanges = false;
		}

		bool HasUnsavedChanges() const override {
			return hasChanges;
		}

	private:
		// Path data
		std::string lastOpenProjectPath;
		std::string defaultProjectPath;
		std::string assetsFolderPath;
		std::string defaultModelRootPath;
		std::string checkpointDir;
		std::string encoderDir;
		std::string vaeDir;
		std::string unetDir;
		std::string loraDir;
		std::string controlnetDir;
		std::string upscaleDir;

		// Backup data
		std::string backupLastOpenProjectPath;
		std::string backupDefaultProjectPath;
		std::string backupAssetsFolderPath;
		std::string backupDefaultModelRootPath;
		std::string backupCheckpointDir;
		std::string backupEncoderDir;
		std::string backupVaeDir;
		std::string backupUnetDir;
		std::string backupLoraDir;
		std::string backupControlnetDir;
		std::string backupUpscaleDir;

		// State tracking
		bool hasChanges = false;

		void LoadFromFilePaths() {
			lastOpenProjectPath = Utils::FilePaths::lastOpenProjectPath;
			defaultProjectPath = Utils::FilePaths::defaultProjectPath;
			assetsFolderPath = Utils::FilePaths::assetsFolderPath;
			defaultModelRootPath = Utils::FilePaths::defaultModelRootPath;
			checkpointDir = Utils::FilePaths::checkpointDir;
			encoderDir = Utils::FilePaths::encoderDir;
			vaeDir = Utils::FilePaths::vaeDir;
			unetDir = Utils::FilePaths::unetDir;
			loraDir = Utils::FilePaths::loraDir;
			controlnetDir = Utils::FilePaths::controlnetDir;
			upscaleDir = Utils::FilePaths::upscaleDir;
		}

		void ApplyToFilePaths() {
			Utils::FilePaths::lastOpenProjectPath = lastOpenProjectPath;
			Utils::FilePaths::defaultProjectPath = defaultProjectPath;
			Utils::FilePaths::assetsFolderPath = assetsFolderPath;
			Utils::FilePaths::defaultModelRootPath = defaultModelRootPath;
			Utils::FilePaths::checkpointDir = checkpointDir;
			Utils::FilePaths::encoderDir = encoderDir;
			Utils::FilePaths::vaeDir = vaeDir;
			Utils::FilePaths::unetDir = unetDir;
			Utils::FilePaths::loraDir = loraDir;
			Utils::FilePaths::controlnetDir = controlnetDir;
			Utils::FilePaths::upscaleDir = upscaleDir;

			Utils::FilePaths::SaveFilepathDefaults();
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