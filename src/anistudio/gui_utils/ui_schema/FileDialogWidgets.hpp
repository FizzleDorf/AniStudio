/*
		d8888          d8b  .d8888b.  888                  888 d8b
	   d88888          Y8P d88P  Y88b 888                  888 Y8P
	  d88P888              Y88b.      888                  888
	 d88P 888 88888b.  888  "Y888b.   888888 888  888  .d88888 888  .d88b.
	d88P  888 888 "88b 888     "Y88b. 888    888  888 d88" 888 888 d88""88b
   d88P   888 888  888 888       "888 888    888  888 888  888 888 888  888
  d8888888888 888  888 888 Y88b  d88P Y88b.  Y88b 888 Y88b 888 888 Y88..88P
 d88P     888 888  888 888  "Y8888P"   "Y888  "Y88888  "Y88888 888  "Y88P"

 * This file is part of AniStudio.
 * Copyright (C) 2025 FizzleDorf (AnimAnon)
 *
 * This software is dual-licensed under the GNU Lesser General Public License v3.0 (LGPL-3.0)
 * and a commercial license. You may choose to use it under either license.
 *
 * For the LGPL-3.0, see the LICENSE-LGPL-3.0.txt file in the repository.
 * For commercial license information, please contact legal@kframe.ai.
 */

#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <filesystem>
#include "ImGuiFileDialog.h"
#include "FileDialogFilters.hpp"
#include "UISchemaUtils.hpp"

namespace UISchema {

	struct FileDialogResult {
		bool wasOkPressed = false;
		std::string selectedPath = "";
		std::string selectedFileName = "";
		std::string fullPath = "";
	};

	using FileDialogCallback = std::function<void(const FileDialogResult&)>;

	class FileDialogWidgets {
	private:
		static inline std::string currentDialogKey = "";
		static inline bool isDialogOpen = false;
		static inline FileDialogCallback currentCallback = nullptr;

	public:
		// Open a file dialog with the specified parameters
		static void OpenFileDialog(
			const std::string& dialogKey,
			const std::string& title,
			const std::string& filters,
			const std::string& path,
			bool isDirectoryMode = false,
			FileDialogCallback callback = nullptr
		) {
			if (isDialogOpen) {
				ImGuiFileDialog::Instance()->Close();
			}

			currentDialogKey = dialogKey;
			currentCallback = callback;
			isDialogOpen = true;

			// Setup dialog configuration
			IGFD::FileDialogConfig config;
			config.path = path;
			config.flags = ImGuiFileDialogFlags_Modal;

			// Special handling for directory selection
			if (isDirectoryMode) {
#ifdef ImGuiFileDialogFlags_SelectDirectory
				config.flags |= ImGuiFileDialogFlags_SelectDirectory;
#endif
			}

			// Open the dialog
			ImGuiFileDialog::Instance()->OpenDialog(
				dialogKey.c_str(),
				title.c_str(),
				isDirectoryMode ? nullptr : filters.c_str(),
				config
			);
		}

		// Process the dialog and return result (call this in your render loop)
		static FileDialogResult ProcessDialog() {
			FileDialogResult result;

			if (!isDialogOpen || currentDialogKey.empty()) {
				return result;
			}

			// Display the dialog
			if (ImGuiFileDialog::Instance()->Display(
				currentDialogKey.c_str(),
				ImGuiWindowFlags_NoCollapse,
				ImVec2(800, 600)
			)) {
				// Dialog was closed
				if (ImGuiFileDialog::Instance()->IsOk()) {
					result.wasOkPressed = true;
					result.selectedFileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
					result.selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
					result.fullPath = ImGuiFileDialog::Instance()->GetFilePathName();
				}

				// Execute callback if provided
				if (currentCallback) {
					currentCallback(result);
				}

				// Clean up
				ImGuiFileDialog::Instance()->Close();
				isDialogOpen = false;
				currentDialogKey = "";
				currentCallback = nullptr;
			}

			return result;
		}

		// Check if any dialog is currently open
		static bool IsDialogOpen() {
			return isDialogOpen;
		}

		// Force close any open dialog
		static void CloseDialog() {
			if (isDialogOpen) {
				ImGuiFileDialog::Instance()->Close();
				isDialogOpen = false;
				currentDialogKey = "";
				currentCallback = nullptr;
			}
		}

		// Simplified file selector widget without table
		static bool RenderFileSelector(
			const std::string& label,
			std::string* value,
			const nlohmann::json& options
		) {
			bool modified = false;

			// Extract options
			std::string mode = GetSchemaValue<std::string>(options, "mode", "file");
			std::string filters = GetSchemaValue<std::string>(options, "filters", "");
			std::string filterName = GetSchemaValue<std::string>(options, "filterName", "Files");
			std::string defaultPath = GetSchemaValue<std::string>(options, "defaultPath", "");
			std::string buttonText = GetSchemaValue<std::string>(options, "buttonText", "Browse...");
			bool showClear = GetSchemaValue<bool>(options, "showClear", true);
			bool showPath = GetSchemaValue<bool>(options, "showPath", true);

			// Create unique ID for this widget
			std::string uniqueId = label + "##" + std::to_string(reinterpret_cast<uintptr_t>(value));

			// Render the browse button
			if (ImGui::Button((buttonText + "##" + uniqueId).c_str())) {
				std::string dialogKey = "FileDialog_" + uniqueId;
				std::string dialogTitle = mode == "directory" ? "Select Directory" : "Select File";
				std::string dialogPath = defaultPath.empty() ? "." : defaultPath;
				bool isDirectoryMode = (mode == "directory");

				// Format filters for ImGuiFileDialog
				std::string formattedFilters = "";
				if (!isDirectoryMode && !filters.empty()) {
					formattedFilters = filterName + "{" + filters + "}";
				}

				OpenFileDialog(
					dialogKey,
					dialogTitle,
					formattedFilters,
					dialogPath,
					isDirectoryMode,
					[value, &modified, isDirectoryMode](const FileDialogResult& result) {
					if (result.wasOkPressed) {
						if (isDirectoryMode) {
							*value = result.selectedPath;
						}
						else {
							*value = result.fullPath;
						}
						modified = true;
					}
				}
				);
			}

			// Show clear button if enabled
			if (showClear) {
				ImGui::SameLine();
				if (ImGui::Button(("Clear##" + uniqueId).c_str())) {
					*value = "";
					modified = true;
				}
			}

			// Show the current path/file if enabled
			if (showPath && !value->empty()) {
				ImGui::SameLine();
				ImGui::Text("%s", value->c_str());
			}
			else if (showPath) {
				ImGui::SameLine();
				ImGui::TextDisabled("No %s selected", mode.c_str());
			}

			return modified;
		}

		// Simplified directory selector
		static bool RenderDirectorySelector(
			const std::string& label,
			std::string* value,
			const std::string& defaultPath = ""
		) {
			nlohmann::json options = {
				{"mode", "directory"},
				{"defaultPath", defaultPath},
				{"buttonText", "Browse..."},
				{"showClear", true},
				{"showPath", true}
			};
			return RenderFileSelector(label, value, options);
		}

		// Simplified file selector
		static bool RenderFileSelectorSimple(
			const std::string& label,
			std::string* value,
			const std::string& filters = "",
			const std::string& filterName = "Files"
		) {
			nlohmann::json options = {
				{"mode", "file"},
				{"filters", filters},
				{"filterName", filterName},
				{"buttonText", "Browse..."},
				{"showClear", true},
				{"showPath", true}
			};
			return RenderFileSelector(label, value, options);
		}

		// Main render function called by UISchema
		static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema) {
			// Extract options
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
				options = schema["ui:options"];
			}

			if (widgetType == "file_selector") {
				return RenderFileSelector(label, value, options);
			}
			else {
				std::cerr << "Unknown file dialog widget type '" << widgetType << "'" << std::endl;
				return false;
			}
		}
	};

} // namespace UISchema