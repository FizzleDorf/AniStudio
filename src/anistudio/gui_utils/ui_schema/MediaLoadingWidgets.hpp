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
#include <GL/glew.h>
#include "ImGuiFileDialog.h"
#include "FileDialogFilters.hpp"
#include "UISchemaUtils.hpp"
#include "ImageUtils.hpp"
#include "OpenGLWrapper.hpp"
#include "PropertyTypes.hpp"
#include <stb_image.h>

namespace UISchema {

	class MediaLoadingWidgets {
	private:
		static inline std::string currentDialogKey = "";
		static inline bool isDialogOpen = false;
		static inline PropertyMap currentProperties;
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

		// Check if a media loading operation resulted in a modification
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

			// Load image using ImageUtils
			int width, height, channels;
			unsigned char* data = Utils::ImageUtils::LoadImageData(imagePath, width, height, channels);
			if (!data) {
				std::cerr << "Failed to load image for preview: " << imagePath << std::endl;
				return 0;
			}

			// Create OpenGL texture using Utils
			GLuint textureID = Utils::OpenGLUtils::GenerateTexture(width, height, channels, data);

			// Cache the texture and dimensions
			imagePreviewCache[imagePath] = textureID;
			imageDimensionsCache[imagePath] = { width, height };

			// Free image data using ImageUtils
			Utils::ImageUtils::FreeImageData(data);

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

		// Open a file dialog for media loading
		static void OpenMediaDialog(
			const std::string& dialogKey,
			const std::string& title,
			const std::string& filters,
			const std::string& path,
			const PropertyMap& properties
		) {
			if (isDialogOpen) {
				ImGuiFileDialog::Instance()->Close();
			}

			currentDialogKey = dialogKey;
			currentProperties = properties;
			isDialogOpen = true;

			// Setup dialog configuration
			IGFD::FileDialogConfig config;
			config.path = path;
			config.flags = ImGuiFileDialogFlags_Modal;

			// Open the dialog
			ImGuiFileDialog::Instance()->OpenDialog(
				dialogKey.c_str(),
				title.c_str(),
				filters.c_str(),
				config
			);
		}

		// Process the dialog and return result
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
					// Get the file path
					std::string fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
					std::string currentPath = ImGuiFileDialog::Instance()->GetCurrentPath();

					// Remove any trailing slashes from currentPath
					while (!currentPath.empty() && (currentPath.back() == '/' || currentPath.back() == '\\')) {
						currentPath.pop_back();
					}

					// Construct full path
					std::string fullPath;
					if (!fileName.empty() && !currentPath.empty()) {
						char pathSeparator = '/';
#ifdef _WIN32
						pathSeparator = '\\';
#endif
						fullPath = currentPath + pathSeparator + fileName;
					}

					// Load the image data using ImageUtils
					int width, height, channels;
					unsigned char* imageData = Utils::ImageUtils::LoadImageData(fullPath, width, height, channels);

					if (imageData) {
						// Create texture using OpenGL wrapper
						GLuint textureID = Utils::OpenGLUtils::GenerateTexture(width, height, channels, imageData);

						// Split path into components
						std::filesystem::path fsPath(fullPath);

						// Update the component properties directly through the PropertyMap
						if (currentProperties.find("fileName") != currentProperties.end()) {
							if (auto fileNamePtr = std::get_if<std::string*>(&currentProperties.at("fileName"))) {
								**fileNamePtr = fsPath.filename().string();
							}
						}
						if (currentProperties.find("filePath") != currentProperties.end()) {
							if (auto filePathPtr = std::get_if<std::string*>(&currentProperties.at("filePath"))) {
								**filePathPtr = fullPath;  // Set full path instead of just directory
							}
						}
						if (currentProperties.find("width") != currentProperties.end()) {
							if (auto widthPtr = std::get_if<int*>(&currentProperties.at("width"))) {
								**widthPtr = width;
							}
						}
						if (currentProperties.find("height") != currentProperties.end()) {
							if (auto heightPtr = std::get_if<int*>(&currentProperties.at("height"))) {
								**heightPtr = height;
							}
						}
						if (currentProperties.find("channels") != currentProperties.end()) {
							if (auto channelsPtr = std::get_if<int*>(&currentProperties.at("channels"))) {
								**channelsPtr = channels;
							}
						}

						std::cout << "Successfully loaded media:" << std::endl;
						std::cout << "  Full Path: " << fullPath << std::endl;
						std::cout << "  File Name: " << fsPath.filename().string() << std::endl;
						std::cout << "  File Path: " << fsPath.parent_path().string() << std::endl;
						std::cout << "  Dimensions: " << width << "x" << height << "x" << channels << std::endl;
						std::cout << "  Texture ID: " << textureID << std::endl;

						// Free the image data since we've stored what we need
						Utils::ImageUtils::FreeImageData(imageData);

						pendingModification = true;
						dialogResult = true;
					}
					else {
						std::cerr << "Failed to load image data from: " << fullPath << std::endl;
					}
				}

				// Clean up dialog
				ImGuiFileDialog::Instance()->Close();
				isDialogOpen = false;
				currentDialogKey = "";
				currentProperties.clear();
			}

			return dialogResult;
		}

		// Enhanced media loader widget that displays and updates component properties
		static bool RenderMediaLoader(
			const std::string& label,
			const nlohmann::json& options,
			const PropertyMap& properties
		) {
			bool modified = false;

			// Extract options
			std::string filters = GetSchemaValue<std::string>(options, "filters", ".png,.jpg,.jpeg,.bmp,.tga");
			std::string filterName = GetSchemaValue<std::string>(options, "filterName", "Image Files");
			std::string defaultPath = GetSchemaValue<std::string>(options, "defaultPath", "");
			std::string buttonText = GetSchemaValue<std::string>(options, "buttonText", "Load Image...");
			std::string browseTooltip = GetSchemaValue<std::string>(options, "browseTooltip", "");

			// Create unique ID for this widget
			std::string uniqueId = label + "##MediaLoader";

			// Show current image information
			ImGui::Text("Image Information:");
			ImGui::Separator();

			// Display current property values
			std::string currentFileName = "";
			std::string currentFilePath = "";
			int currentWidth = 0, currentHeight = 0, currentChannels = 0;

			if (properties.find("fileName") != properties.end()) {
				if (auto fileNamePtr = std::get_if<std::string*>(&properties.at("fileName"))) {
					currentFileName = **fileNamePtr;
					ImGui::Text("File Name: %s", currentFileName.empty() ? "(none)" : currentFileName.c_str());
				}
			}

			if (properties.find("filePath") != properties.end()) {
				if (auto filePathPtr = std::get_if<std::string*>(&properties.at("filePath"))) {
					currentFilePath = **filePathPtr;
					ImGui::Text("File Path: %s", currentFilePath.empty() ? "(none)" : currentFilePath.c_str());
				}
			}

			// Display dimensions if available
			bool hasValidDimensions = false;
			if (properties.find("width") != properties.end()) {
				if (auto widthPtr = std::get_if<int*>(&properties.at("width"))) {
					currentWidth = **widthPtr;
					hasValidDimensions = currentWidth > 0;
				}
			}
			if (properties.find("height") != properties.end()) {
				if (auto heightPtr = std::get_if<int*>(&properties.at("height"))) {
					currentHeight = **heightPtr;
				}
			}
			if (properties.find("channels") != properties.end()) {
				if (auto channelsPtr = std::get_if<int*>(&properties.at("channels"))) {
					currentChannels = **channelsPtr;
				}
			}

			if (hasValidDimensions) {
				ImGui::Text("Dimensions: %dx%dx%d", currentWidth, currentHeight, currentChannels);

				// Show preview if we have a valid file path
				// Since filePath now contains the full path, use it directly
				std::string previewPath = currentFilePath;

				if (!previewPath.empty() && std::filesystem::exists(previewPath)) {
					RenderImagePreview(previewPath, 128.0f);
				}
			}
			else {
				ImGui::Text("Dimensions: (no image loaded)");
			}

			ImGui::Separator();

			// Determine dialog path
			std::string actualDialogPath = defaultPath;
			if (!currentFilePath.empty()) {
				actualDialogPath = currentFilePath;
			}
			if (actualDialogPath.empty()) {
				actualDialogPath = ".";
			}

			// Render the browse button
			if (ImGui::Button((buttonText + "##" + uniqueId).c_str())) {
				std::string dialogKey = "MediaLoader_" + uniqueId;
				std::string dialogTitle = "Load Image";

				// Format filters for ImGuiFileDialog
				std::string formattedFilters = filterName + "{" + filters + "}";

				OpenMediaDialog(
					dialogKey,
					dialogTitle,
					formattedFilters,
					actualDialogPath,
					properties
				);
			}

			// Add browse button tooltip
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				if (!browseTooltip.empty()) {
					ImGui::Text("%s", browseTooltip.c_str());
				}
				else {
					ImGui::Text("Browse to load an image file");
				}
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}

			return modified;
		}

		// Main render function - matches what UISchema expects but without callback
		static bool Render(const std::string& label, const std::string& widgetType, const nlohmann::json& schema, const PropertyMap& properties) {
			// Extract options
			nlohmann::json options = {};
			if (schema.contains("ui:options") && schema["ui:options"].is_object()) {
				options = schema["ui:options"];
			}

			if (widgetType == "media_loader") {
				return RenderMediaLoader(label, options, properties);
			}
			else {
				std::cerr << "Unknown media loading widget type '" << widgetType << "'" << std::endl;
				return false;
			}
		}
	};

} // namespace UISchema