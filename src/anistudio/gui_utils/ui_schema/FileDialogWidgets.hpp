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

		// FIXED: Process the dialog and return result (call this in your render loop)
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

					// FIXED: Get the components separately and construct properly
					std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string currentPath = ImGuiFileDialog::Instance()->GetCurrentPath();

					// Remove any trailing slashes from currentPath
					while (!currentPath.empty() && (currentPath.back() == '/' || currentPath.back() == '\\')) {
						currentPath.pop_back();
					}

					// Set the results properly
					result.selectedFileName = fileName;
					result.selectedPath = currentPath;

					// FIXED: Construct fullPath manually to avoid duplication
					if (!fileName.empty() && !currentPath.empty()) {
						// Use appropriate path separator for the platform
						char pathSeparator = '/';
#ifdef _WIN32
						pathSeparator = '\\';
#endif
						result.fullPath = currentPath + pathSeparator + fileName;
					}
					else if (!currentPath.empty()) {
						// Directory mode - use the current path as full path
						result.fullPath = currentPath;
					}
					else {
						result.fullPath = fileName; // Fallback
					}

					// Debug output to verify the fix
					std::cout << "FileDialog Result:" << std::endl;
					std::cout << "  Selected Path: " << result.selectedPath << std::endl;
					std::cout << "  Selected Filename: " << result.selectedFileName << std::endl;
					std::cout << "  Full Path: " << result.fullPath << std::endl;
				}

				// Execute callback if provided
				if (currentCallback) {
					currentCallback(result);
				}

				// Clean up and PROPERLY clear search filter
				ImGuiFileDialog::Instance()->Close();

				// Try different methods to clear search/filter state depending on version
				try {
					// Clear any search/filter state
					ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByFullName, "", ImVec4(0, 0, 0, 0));

					// Force refresh the dialog state
					ImGuiFileDialog::Instance()->OpenDialog("__dummy__", "Clear", nullptr, IGFD::FileDialogConfig{});
					ImGuiFileDialog::Instance()->Close();
				}
				catch (...) {
					// Ignore any errors - just ensure dialog is closed
				}

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

		// Simplified file selector widget - always shows path above, wraps text
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
			std::string dialogDefaultPath = GetSchemaValue<std::string>(options, "dialogDefaultPath", defaultPath);
			std::string buttonText = GetSchemaValue<std::string>(options, "buttonText", "Browse...");
			std::string resetButtonText = GetSchemaValue<std::string>(options, "resetButtonText", "Clear");
			std::string browseTooltip = GetSchemaValue<std::string>(options, "browseTooltip", "");

			// Initialize value to default path if empty and default exists
			if (value->empty() && !defaultPath.empty()) {
				*value = defaultPath;
				modified = true;
			}

			// Create unique ID for this widget
			std::string uniqueId = label + "##" + std::to_string(reinterpret_cast<uintptr_t>(value));

			// ALWAYS show current path above buttons with text wrapping
			if (!value->empty()) {
				// Show the current path/filename with wrapping
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

				// Smart path truncation for very long paths
				std::string displayPath = *value;
				if (displayPath.length() > 80) {
					std::filesystem::path p(displayPath);
					std::string filename = p.filename().string();
					std::string directory = p.parent_path().string();

					if (directory.length() > 50) {
						// Truncate directory but keep beginning and end
						std::string truncated = directory.substr(0, 25) + "..." + directory.substr(directory.length() - 22);
						displayPath = truncated + "/" + filename;
					}
				}

				ImGui::TextWrapped("%s", displayPath.c_str());
				ImGui::PopStyleColor();
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::TextWrapped("No %s selected", mode.c_str());
				ImGui::PopStyleColor();
			}
			ImGui::Spacing();

			// Determine the actual dialog path to use - prefer last selected path
			std::string actualDialogPath;
			if (!value->empty()) {
				// Use the directory of the current file (last selected path)
				std::filesystem::path currentPath(*value);
				if (std::filesystem::exists(currentPath.parent_path())) {
					actualDialogPath = currentPath.parent_path().string();
				}
			}

			// Fallback to dialog default path, then current directory
			if (actualDialogPath.empty()) {
				actualDialogPath = dialogDefaultPath;
			}
			if (actualDialogPath.empty()) {
				actualDialogPath = ".";
			}

			// Render the browse button
			if (ImGui::Button((buttonText + "##" + uniqueId).c_str())) {
				std::string dialogKey = "FileDialog_" + uniqueId;
				std::string dialogTitle = mode == "directory" ? "Select Directory" : "Select File";
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
					actualDialogPath,
					isDirectoryMode,
					[value, &modified, isDirectoryMode](const FileDialogResult& result) {
					if (result.wasOkPressed) {
						if (isDirectoryMode) {
							*value = result.selectedPath;
						}
						else {
							// Split the full path into directory and filename
							std::filesystem::path fullPath(result.fullPath);
							std::string directory = fullPath.parent_path().string();
							std::string filename = fullPath.filename().string();

							// Set the value to just the filename
							*value = filename;

							std::cout << "FileDialog split result:" << std::endl;
							std::cout << "  Full Path: " << result.fullPath << std::endl;
							std::cout << "  Directory: " << directory << std::endl;
							std::cout << "  Filename: " << filename << std::endl;
							std::cout << "  Setting value to: " << filename << std::endl;
						}
						modified = true;
					}
				}
				);
			}
			// Add browse button tooltip based on schema or default
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				if (!browseTooltip.empty()) {
					ImGui::Text("%s", browseTooltip.c_str());
				}
				else if (mode == "directory") {
					ImGui::Text("Browse to select a directory path");
				}
				else {
					ImGui::Text("Browse to select a file");
				}
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			// Show reset button if default path exists
			if (!defaultPath.empty()) {
				ImGui::SameLine();
				if (ImGui::Button((resetButtonText + "##" + uniqueId).c_str())) {
					*value = defaultPath;  // Reset to default path
					modified = true;
				}
				// Add tooltip for reset button
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);  // Set explicit wrap width
					ImGui::Text("Reset to default: %s", defaultPath.c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			}
			else {
				// Only show clear button if no default path exists
				ImGui::SameLine();
				if (ImGui::Button(("Clear##" + uniqueId).c_str())) {
					*value = "";
					modified = true;
				}
			}

			return modified;
		}

		// Enhanced directory selector with default path support
		static bool RenderDirectorySelector(
			const std::string& label,
			std::string* value,
			const std::string& defaultPath = "",
			const std::string& dialogDefaultPath = ""
		) {
			nlohmann::json options = {
				{"mode", "directory"},
				{"defaultPath", defaultPath},
				{"dialogDefaultPath", dialogDefaultPath.empty() ? defaultPath : dialogDefaultPath},
				{"buttonText", "Browse..."},
				{"resetButtonText", "Reset"}
			};
			return RenderFileSelector(label, value, options);
		}

		// Enhanced file selector with default path support
		static bool RenderFileSelectorSimple(
			const std::string& label,
			std::string* value,
			const std::string& filters = "",
			const std::string& filterName = "Files",
			const std::string& defaultPath = "",
			const std::string& dialogDefaultPath = ""
		) {
			nlohmann::json options = {
				{"mode", "file"},
				{"filters", filters},
				{"filterName", filterName},
				{"defaultPath", defaultPath},
				{"dialogDefaultPath", dialogDefaultPath.empty() ? defaultPath : dialogDefaultPath},
				{"buttonText", "Browse..."},
				{"resetButtonText", "Reset"}
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