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
#include "AssetManager.hpp"
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

		// Create OpenGL texture manually (since Utils::OpenGLUtils doesn't exist)
		static GLuint CreateTextureFromImageData(int width, int height, int channels, unsigned char* data) {
			if (!data || width <= 0 || height <= 0) {
				return 0;
			}

			GLuint textureID;
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);

			// Set texture parameters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			// Upload texture data
			GLenum format = GL_RGB;
			if (channels == 4) format = GL_RGBA;
			else if (channels == 1) format = GL_RED;

			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			glBindTexture(GL_TEXTURE_2D, 0);
			return textureID;
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

			// Create OpenGL texture manually
			GLuint textureID = CreateTextureFromImageData(width, height, channels, data);

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

					std::cout << "=========================================" << std::endl;
					std::cout << "MediaLoadingWidgets::ProcessDialog()" << std::endl;
					std::cout << "File selected: " << fullPath << std::endl;
					std::cout << "File exists: " << std::filesystem::exists(fullPath) << std::endl;

					// Debug: Show what properties we're supposed to update
					std::cout << "Properties to update:" << std::endl;
					for (const auto&[key, variant] : currentProperties) {
						std::cout << "  - " << key << std::endl;
					}

					// Load the image data using ImageUtils
					int width, height, channels;
					unsigned char* imageData = Utils::ImageUtils::LoadImageData(fullPath, width, height, channels);

					if (imageData) {
						std::cout << "Image loaded successfully:" << std::endl;
						std::cout << "  Dimensions: " << width << "x" << height << "x" << channels << std::endl;

						// Create texture using our local function
						GLuint textureID = CreateTextureFromImageData(width, height, channels, imageData);

						// Split path into components
						std::filesystem::path fsPath(fullPath);

						// Update the component properties directly through the PropertyMap

						if (currentProperties.find("fileName") != currentProperties.end()) {
							if (auto fileNamePtr = std::get_if<std::string*>(&currentProperties.at("fileName"))) {
								std::cout << "Setting fileName to: " << fsPath.filename().string() << std::endl;
								**fileNamePtr = fsPath.filename().string();
							}
							else {
								std::cout << "ERROR: fileName property is not a string pointer!" << std::endl;
							}
						}
						else {
							std::cout << "WARNING: fileName not in properties map!" << std::endl;
						}

						if (currentProperties.find("filePath") != currentProperties.end()) {
							if (auto filePathPtr = std::get_if<std::string*>(&currentProperties.at("filePath"))) {
								std::cout << "Setting filePath to: " << fullPath << std::endl;
								**filePathPtr = fullPath;
							}
							else {
								std::cout << "ERROR: filePath property is not a string pointer!" << std::endl;
							}
						}
						else {
							std::cout << "WARNING: filePath not in properties map!" << std::endl;
						}

						if (currentProperties.find("width") != currentProperties.end()) {
							if (auto widthPtr = std::get_if<int*>(&currentProperties.at("width"))) {
								std::cout << "Setting width to: " << width << std::endl;
								**widthPtr = width;
							}
							else {
								std::cout << "ERROR: width property is not an int pointer!" << std::endl;
							}
						}
						else {
							std::cout << "WARNING: width not in properties map!" << std::endl;
						}

						if (currentProperties.find("height") != currentProperties.end()) {
							if (auto heightPtr = std::get_if<int*>(&currentProperties.at("height"))) {
								std::cout << "Setting height to: " << height << std::endl;
								**heightPtr = height;
							}
							else {
								std::cout << "ERROR: height property is not an int pointer!" << std::endl;
							}
						}
						else {
							std::cout << "WARNING: height not in properties map!" << std::endl;
						}

						if (currentProperties.find("channels") != currentProperties.end()) {
							if (auto channelsPtr = std::get_if<int*>(&currentProperties.at("channels"))) {
								std::cout << "Setting channels to: " << channels << std::endl;
								**channelsPtr = channels;
							}
							else {
								std::cout << "ERROR: channels property is not an int pointer!" << std::endl;
							}
						}
						else {
							std::cout << "WARNING: channels not in properties map!" << std::endl;
						}

						std::cout << "=========================================" << std::endl;

						// Free the image data since we've stored what we need
						Utils::ImageUtils::FreeImageData(imageData);

						pendingModification = true;
						dialogResult = true;
					}
					else {
						std::cerr << "ERROR: Failed to load image data from: " << fullPath << std::endl;
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

			// Check if the dialog resulted in a modification
			if (WasModified()) {
				modified = true;
				std::cout << "Media loader: Properties were modified via dialog" << std::endl;

				// Debug output to verify width/height were set
				if (properties.find("width") != properties.end()) {
					if (auto widthPtr = std::get_if<int*>(&properties.at("width"))) {
						std::cout << "Width in properties: " << **widthPtr << std::endl;
					}
				}
				if (properties.find("height") != properties.end()) {
					if (auto heightPtr = std::get_if<int*>(&properties.at("height"))) {
						std::cout << "Height in properties: " << **heightPtr << std::endl;
					}
				}
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