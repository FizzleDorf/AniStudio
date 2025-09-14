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
#include "UISchemaContext.hpp"
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
						char pathSeparator = '/';
#ifdef _WIN32
						pathSeparator = '\\';
#endif
						result.fullPath = currentPath + pathSeparator + fileName;
					}
					else if (!currentPath.empty()) {
						result.fullPath = currentPath;
					}
					else {
						result.fullPath = fileName;
					}

					// Execute callback if provided
					if (currentCallback) {
						currentCallback(result);
						pendingModification = true;
						dialogResult = true;
					}
				}

				// Clean up dialog
				ImGuiFileDialog::Instance()->Close();
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
			const nlohmann::json& options,
			const UIRenderContext& context
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

			// Extract property name from label for ID generation
			std::string propertyName = label;
			size_t hashPos = label.find("##");
			if (hashPos != std::string::npos) {
				propertyName = label.substr(0, hashPos);
				if (hashPos == 0) {
					std::string uniquePart = label.substr(2);
					size_t underscorePos = uniquePart.find('_');
					if (underscorePos != std::string::npos) {
						size_t secondUnderscorePos = uniquePart.find('_', underscorePos + 1);
						if (secondUnderscorePos != std::string::npos) {
							propertyName = uniquePart.substr(underscorePos + 1, secondUnderscorePos - underscorePos - 1);
						}
					}
				}
			}

			// Show image preview FIRST if this is actually an image file
			if (!value->empty() && std::filesystem::exists(*value)) {
				std::filesystem::path filePath(*value);
				std::string extension = filePath.extension().string();
				std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

				if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
					extension == ".bmp" || extension == ".tga") {
					RenderImagePreview(*value, 200.0f);

					// Add clear button for image
					std::string clearImageButtonId = context.GenerateWidgetId(propertyName, "clear_image");
					std::string clearImageButtonLabel = "Clear Image##" + clearImageButtonId;
					if (ImGui::Button(clearImageButtonLabel.c_str())) {
						*value = "";
						modified = true;
					}

					ImGui::Spacing();
				}
			}

			// Show current path above buttons with text wrapping
			if (!value->empty()) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

				// Show just filename for ALL files, with full path in tooltip
				std::filesystem::path filePath(*value);
				std::string displayName = filePath.filename().string();

				if (displayName.empty()) {
					displayName = *value; // Fallback to full path if no filename
				}

				ImGui::TextWrapped("%s", displayName.c_str());

				// Show full path in tooltip for ALL file types
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 50.0f);
					ImGui::Text("Full path: %s", value->c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::PopStyleColor();
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				ImGui::TextWrapped("No %s selected", mode.c_str());
				ImGui::PopStyleColor();
			}
			ImGui::Spacing();

			// Determine the actual dialog path to use
			std::string actualDialogPath;
			if (!value->empty()) {
				std::filesystem::path currentPath(*value);
				if (std::filesystem::exists(currentPath.parent_path())) {
					actualDialogPath = currentPath.parent_path().string();
				}
			}

			if (actualDialogPath.empty()) {
				actualDialogPath = dialogDefaultPath;
			}
			if (actualDialogPath.empty()) {
				actualDialogPath = ".";
			}

			// Generate unique button IDs using the context information
			std::string browseButtonId = context.GenerateWidgetId(propertyName, "browse");
			std::string browseButtonLabel = buttonText + "##" + browseButtonId;

			// Render the browse button
			if (ImGui::Button(browseButtonLabel.c_str())) {
				std::string dialogKey = context.GenerateWidgetId(propertyName, "file_dialog");
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
					[value, isDirectoryMode](const FileDialogResult& result) {
					if (result.wasOkPressed) {
						if (isDirectoryMode) {
							*value = result.selectedPath;
						}
						else {
							// ALWAYS store the full path for ALL file types
							*value = result.fullPath;
							std::cout << "File selected: " << result.fullPath << std::endl;
						}
					}
				}
				);
			}

			// Add browse button tooltip
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

			// Show reset/clear button
			if (!defaultPath.empty()) {
				ImGui::SameLine();
				std::string resetButtonId = context.GenerateWidgetId(propertyName, "reset");
				std::string resetButtonLabel = resetButtonText + "##" + resetButtonId;

				if (ImGui::Button(resetButtonLabel.c_str())) {
					*value = defaultPath;
					modified = true;
				}

				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::Text("Reset to default: %s", defaultPath.c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			}
			else {
				ImGui::SameLine();
				std::string clearButtonId = context.GenerateWidgetId(propertyName, "clear");
				std::string clearButtonLabel = "Clear##" + clearButtonId;

				if (ImGui::Button(clearButtonLabel.c_str())) {
					*value = "";
					modified = true;
				}
			}

			return modified;
		}

		// Main render function called by UISchema
		static bool Render(const std::string& label, std::string* value, const std::string& widgetType, const nlohmann::json& schema, const UIRenderContext& context) {
			// Extract options
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
				options = schema["ui:options"];
			}

			if (widgetType == "file_selector") {
				return RenderFileSelector(label, value, options, context);
			}
			else {
				std::cerr << "Unknown file dialog widget type '" << widgetType << "'" << std::endl;
				return false;
			}
		}
	};

} // namespace UISchema