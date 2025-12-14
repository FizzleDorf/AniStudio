#pragma once

#include "BaseTabObject.hpp"
#include "ImGuiFileDialog.h"
#include "FilePaths.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

namespace Settings {

	class PathsSettingsTab : public BaseTabObject {
	public:
		PathsSettingsTab() : BaseTabObject("Paths", "Application") {
			// Define path categories and their display names
			InitializePathCategories();
			LoadFromFilePaths();
			LoadSettings();
			CreateBackup();
		}

		void RenderUI() override {
			if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
				// Render each category
				for (const auto& category : pathCategories) {
					RenderCategory(category.first, category.second);
				}

				ImGui::Separator();

				// Special button for resetting model paths
				auto modelRootIt = pathMap.find("ModelRoot");
				if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
					if (ImGui::Button("Reset Model Paths to Root")) {
						ResetModelPathsToRoot(modelRootIt->second);
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(Will update all model subdirectories)");
				}

				ImGui::Separator();
				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		void RenderFilteredUI(const std::set<std::string>& selectedCategories) override {
			if (ImGui::BeginChild("PathsSettings", ImVec2(0, 0), false)) {
				// Render only selected categories
				for (const auto& category : pathCategories) {
					if (ShouldRenderCategory(category.first, selectedCategories)) {
						RenderCategory(category.first, category.second);
					}
				}

				// Special button for resetting model paths
				auto modelRootIt = pathMap.find("ModelRoot");
				if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
					if (ImGui::Button("Reset Model Paths to Root")) {
						ResetModelPathsToRoot(modelRootIt->second);
					}
				}

				RenderActionButtons();
			}
			ImGui::EndChild();
		}

		bool SaveSettings() override {
			try {
				nlohmann::json j;

				// Save all paths dynamically
				for (const auto&[key, value] : pathMap) {
					j[key] = value;
				}

				// Also save category information
				nlohmann::json categoriesJson;
				for (const auto&[categoryName, pathKeys] : pathCategories) {
					categoriesJson[categoryName] = pathKeys;
				}
				j["_categories"] = categoriesJson;

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

				// Load all paths dynamically
				for (auto it = j.begin(); it != j.end(); ++it) {
					// Skip metadata keys starting with underscore
					if (it.key().rfind("_", 0) == 0) continue;

					if (it.value().is_string()) {
						pathMap[it.key()] = it.value();
					}
				}

				// Load categories if they exist
				if (j.contains("_categories")) {
					const auto& categoriesJson = j["_categories"];
					for (auto it = categoriesJson.begin(); it != categoriesJson.end(); ++it) {
						if (it.value().is_array()) {
							std::vector<std::string> keys;
							for (const auto& key : it.value()) {
								keys.push_back(key);
							}
							pathCategories[it.key()] = keys;
						}
					}
				}

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
			// Clear all paths
			for (auto&[key, value] : pathMap) {
				value.clear();
			}
			hasChanges = true;
		}

		void CreateBackup() override {
			backupPathMap = pathMap;
		}

		void RestoreFromBackup() override {
			pathMap = backupPathMap;
			hasChanges = false;
		}

		bool HasUnsavedChanges() const override {
			return hasChanges;
		}

	private:
		// Dynamic path storage
		std::map<std::string, std::string> pathMap;
		std::map<std::string, std::string> backupPathMap;

		// Path categories for UI organization
		std::map<std::string, std::vector<std::string>> pathCategories;

		// State tracking
		bool hasChanges = false;

		void InitializePathCategories() {
			// General paths
			pathCategories["General Paths"] = {
				"LastOpenProject",
				"DefaultProject",
				"AssetsFolder",
				"OutputFolder",
				"VirtualEnv",
				"Scripts",
				"Plugins",
				"ImguiState"
			};

			// Model paths
			pathCategories["Model Paths"] = {
				"ModelRoot",
				"Checkpoint",
				"Encoder",
				"Vae",
				"Unet",
				"Lora",
				"ControlNet",
				"Upscale",
				"Embed"
			};

			// CLIP specific file paths (not directories)
			pathCategories["CLIP Files"] = {
				"ClipL",
				"ClipG",
				"T5XXL"
			};
		}

		void LoadFromFilePaths() {
			auto& filePaths = Utils::FilePaths::GetInstance();

			// Load all paths from FilePaths into our map
			// We'll iterate through all categories to get all possible paths
			for (const auto&[category, keys] : pathCategories) {
				for (const auto& key : keys) {
					pathMap[key] = filePaths.GetPath(key.c_str());
				}
			}

			// Also load any additional paths that might exist in FilePaths
			// (This would require FilePaths to expose its internal map, which it doesn't currently)
			// For now, we'll just work with what we've defined in categories
		}

		void ApplyToFilePaths() {
			auto& filePaths = Utils::FilePaths::GetInstance();

			// Apply all paths from our map to FilePaths
			for (const auto&[key, value] : pathMap) {
				filePaths.SetPath(key.c_str(), value.c_str());
			}

			// If ModelRoot was changed, ensure model subdirectories are properly structured
			auto modelRootIt = pathMap.find("ModelRoot");
			if (modelRootIt != pathMap.end() && !modelRootIt->second.empty()) {
				// Call SetByModelRoot to update all model subdirectories
				filePaths.SetByModelRoot();
			}

			// Save to file
			filePaths.SaveFilepathDefaults();
		}

		void RenderCategory(const std::string& categoryName, const std::vector<std::string>& pathKeys) {
			ImGui::Text("%s", categoryName.c_str());
			ImGui::Spacing();

			if (ImGui::BeginTable((categoryName + "Table").c_str(), 3,
				ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {

				ImGui::TableSetupColumn("Path Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("Browse", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Full Path", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				for (const auto& key : pathKeys) {
					auto it = pathMap.find(key);
					if (it != pathMap.end()) {
						RenderPathRow(key.c_str(), it->second, key);
					}
				}

				ImGui::EndTable();
			}
			ImGui::Separator();
		}

		void RenderPathRow(const char* label, std::string& path, const std::string& pathKey) {
			ImGui::TableNextRow();

			// Display name (convert camelCase to Title Case)
			ImGui::TableNextColumn();
			std::string displayName = FormatDisplayName(label);
			ImGui::TextUnformatted(displayName.c_str());

			// Browse button
			ImGui::TableNextColumn();
			std::string buttonID = std::string("...##") + label;

			// Determine if this is a directory or file selector
			bool isFileSelector = (pathKey == "ClipL" || pathKey == "ClipG" || pathKey == "T5XXL");

			if (ImGui::Button(buttonID.c_str())) {
				IGFD::FileDialogConfig config;
				config.path = path.empty() ? "." : path;
				config.flags = ImGuiFileDialogFlags_Modal;
				std::string dialogID = std::string("ChoosePath##") + label;

				if (isFileSelector) {
					// For CLIP files, use file selector
					ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(),
						"Select File",
						"All Files{.*},.safetensors{.safetensors},.ckpt{.ckpt},.pth{.pth}",
						config);
				}
				else {
					// For directories, use directory selector
					ImGuiFileDialog::Instance()->OpenDialog(dialogID.c_str(),
						"Select Directory",
						nullptr,
						config);
				}
			}

			std::string dialogID = std::string("ChoosePath##") + label;
			if (ImGuiFileDialog::Instance()->Display(dialogID.c_str(), 32, ImVec2(500, 400))) {
				if (ImGuiFileDialog::Instance()->IsOk()) {
					std::string selectedPath;
					if (isFileSelector) {
						selectedPath = ImGuiFileDialog::Instance()->GetFilePathName();
					}
					else {
						selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
					}

					if (!selectedPath.empty() && selectedPath != path) {
						path = selectedPath;
						hasChanges = true;

						// If this is ModelRoot, automatically update model subdirectories
						if (pathKey == "ModelRoot" && !path.empty()) {
							UpdateModelSubdirectories(path);
						}
					}
				}
				ImGuiFileDialog::Instance()->Close();
			}

			// Reset button
			ImGui::SameLine();
			std::string resetID = std::string("R##") + label;
			if (ImGui::Button(resetID.c_str())) {
				if (!path.empty()) {
					path.clear();
					hasChanges = true;
				}
			}

			// Path display
			ImGui::TableNextColumn();
			if (path.empty()) {
				ImGui::TextDisabled("(not set)");
			}
			else {
				// Truncate path if too long
				std::string displayPath = path;
				if (displayPath.length() > 60) {
					displayPath = "..." + displayPath.substr(displayPath.length() - 57);
				}
				ImGui::Text("%s", displayPath.c_str());
				if (ImGui::IsItemHovered() && path.length() > 60) {
					ImGui::SetTooltip("%s", path.c_str());
				}
			}
		}

		void ResetModelPathsToRoot(const std::string& modelRoot) {
			if (modelRoot.empty()) return;

			UpdateModelSubdirectories(modelRoot);
			hasChanges = true;
		}

		void UpdateModelSubdirectories(const std::string& modelRoot) {
			// Define model subdirectory mappings
			std::map<std::string, std::string> modelSubdirs = {
				{"Checkpoint", "/checkpoints"},
				{"Encoder", "/clip"},
				{"Vae", "/vae"},
				{"Unet", "/unet"},
				{"Lora", "/loras"},
				{"ControlNet", "/controlnet"},
				{"Upscale", "/upscale_models"},
				{"Embed", "/embeddings"}
			};

			// Update all model subdirectories
			for (const auto&[key, subdir] : modelSubdirs) {
				pathMap[key] = modelRoot + subdir;
			}

			// Also update CLIP file paths
			std::string encoderDir = modelRoot + "/clip";
			pathMap["ClipL"] = encoderDir + "/clip_l.safetensors";
			pathMap["ClipG"] = encoderDir + "/clip_g.safetensors";
			pathMap["T5XXL"] = encoderDir + "/t5xxl.safetensors";
		}

		std::string FormatDisplayName(const std::string& key) {
			std::string result;
			bool lastWasLower = false;

			for (char c : key) {
				if (isupper(c) && lastWasLower) {
					result += ' ';
					result += c;
				}
				else if (c == '_' || c == '-') {
					result += ' ';
				}
				else {
					result += c;
				}
				lastWasLower = islower(c);
			}

			// Capitalize first letter
			if (!result.empty()) {
				result[0] = toupper(result[0]);
			}

			return result;
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