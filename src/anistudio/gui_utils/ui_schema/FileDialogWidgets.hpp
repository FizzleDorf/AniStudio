#pragma once

#include <imgui.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <filesystem>
#include <GL/glew.h>
#include "ImGuiFileDialog.h"
#include "FileDialogFilters.hpp"
#include "UISchemaUtils.hpp"
#include <stb_image.h>

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
		static inline bool pendingModification = false;

		// Cache for image previews
		static inline std::unordered_map<std::string, GLuint> imagePreviewCache;
		static inline std::unordered_map<std::string, std::pair<int, int>> imageDimensionsCache;

	public:
		// Clean up preview textures
		static void CleanupPreviews() {
			for (auto&[path, textureID] : imagePreviewCache) {
				if (textureID != 0) {
					glDeleteTextures(1, &textureID);
				}
			}
			imagePreviewCache.clear();
			imageDimensionsCache.clear();
		}

		// Check if a file dialog operation resulted in a modification
		static bool WasModified() {
			bool result = pendingModification;
			pendingModification = false;
			return result;
		}

		// Create preview texture for image
		static GLuint CreateImagePreview(const std::string& imagePath) {
			// Check cache first
			if (imagePreviewCache.count(imagePath)) {
				return imagePreviewCache[imagePath];
			}

			if (!std::filesystem::exists(imagePath)) {
				return 0;
			}

			// Load image
			int width, height, channels;
			unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
			if (!data) {
				std::cerr << "Failed to load image for preview: " << imagePath << std::endl;
				return 0;
			}

			// Create OpenGL texture
			GLuint textureID;
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			// Set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Upload texture data
			GLenum format = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_RED;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			// Cache the texture and dimensions
			imagePreviewCache[imagePath] = textureID;
			imageDimensionsCache[imagePath] = { width, height };

			// Free image data
			stbi_image_free(data);

			std::cout << "Created preview texture for: " << imagePath << " (" << width << "x" << height << ")" << std::endl;
			return textureID;
		}

		// Render image preview
		static void RenderImagePreview(const std::string& imagePath, float maxSize = 256.0f) {
			if (imagePath.empty() || !std::filesystem::exists(imagePath)) {
				return;
			}

			GLuint textureID = CreateImagePreview(imagePath);
			if (textureID == 0) {
				return;
			}

			// Get cached dimensions
			auto dimIt = imageDimensionsCache.find(imagePath);
			if (dimIt == imageDimensionsCache.end()) {
				return;
			}

			int width = dimIt->second.first;
			int height = dimIt->second.second;

			// Calculate preview size maintaining aspect ratio
			float aspectRatio = static_cast<float>(width) / height;
			ImVec2 previewSize;
			if (aspectRatio > 1.0f) {
				previewSize = ImVec2(maxSize, maxSize / aspectRatio);
			}
			else {
				previewSize = ImVec2(maxSize * aspectRatio, maxSize);
			}

			// Render the preview
			ImGui::Text("Preview (%dx%d):", width, height);
			ImGui::Image((ImTextureID)(intptr_t)textureID, previewSize);
		}

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
		static bool ProcessDialog() {
			bool dialogResult = false;

			if (!isDialogOpen || currentDialogKey.empty()) {
				return dialogResult;
			}

			// Display the dialog
			if (ImGuiFileDialog::Instance()->Display(
				currentDialogKey.c_str(),
				ImGuiWindowFlags_NoCollapse,
				ImVec2(800, 600)
			)) {
				// Dialog was closed
				if (ImGuiFileDialog::Instance()->IsOk()) {
					FileDialogResult result;
					result.wasOkPressed = true;

					// Get the components separately and construct properly
					std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string currentPath = ImGuiFileDialog::Instance()->GetCurrentPath();

					// Remove any trailing slashes from currentPath
					while (!currentPath.empty() && (currentPath.back() == '/' || currentPath.back() == '\\')) {
						currentPath.pop_back();
					}

					// Set the results properly
					result.selectedFileName = fileName;
					result.selectedPath = currentPath;

					// Construct fullPath manually to avoid duplication
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

					// Execute callback if provided
					if (currentCallback) {
						currentCallback(result);
						pendingModification = true;
						dialogResult = true;
					}
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

			return dialogResult;
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

		// Enhanced file selector with image preview and proper path handling
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

			// Check if this component needs full path storage
			bool needsFullPath = options.contains("component_type") &&
				options["component_type"] == "InputImageComponent";

			// Check if this is an image file selector for preview
			bool isImageSelector = (filters.find(".png") != std::string::npos ||
				filters.find(".jpg") != std::string::npos ||
				filters.find(".jpeg") != std::string::npos ||
				filters.find(".bmp") != std::string::npos ||
				filters.find(".tga") != std::string::npos ||
				needsFullPath);

			// Initialize value to default path if empty and default exists
			if (value->empty() && !defaultPath.empty()) {
				*value = defaultPath;
				modified = true;
			}

			// Create unique ID for this widget
			std::string uniqueId = label + "##" + std::to_string(reinterpret_cast<uintptr_t>(value));

			// Show image preview FIRST if this is an image and we have a valid path
			if (isImageSelector && !value->empty() && std::filesystem::exists(*value)) {
				RenderImagePreview(*value, 200.0f);
				ImGui::Spacing();
			}

			// Show current path above buttons with text wrapping
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
					[value, isDirectoryMode, needsFullPath](const FileDialogResult& result) {
					if (result.wasOkPressed) {
						if (isDirectoryMode) {
							*value = result.selectedPath;
						}
						else {
							if (needsFullPath) {
								// Store full path for InputImageComponent
								*value = result.fullPath;
								std::cout << "InputImageComponent: Setting FULL PATH: " << result.fullPath << std::endl;
							}
							else {
								// For other components, store just the filename
								std::filesystem::path fullPath(result.fullPath);
								std::string filename = fullPath.filename().string();
								*value = filename;
								std::cout << "Other component: Setting filename: " << filename << std::endl;
							}
						}
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
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
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