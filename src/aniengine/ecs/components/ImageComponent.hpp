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

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include "AssetTypes.hpp"
#include <string>
#include <memory>

namespace ECS {
	struct ImageComponent : public BaseComponent {
		std::string fileName = "AniStudio";                  // Default file name
		std::string filePath =
			!Utils::FilePaths::outputFolderPath.empty()
			? Utils::FilePaths::outputFolderPath			 // Output folder if one is found
			: Utils::FilePaths::defaultProjectPath;			 // Directory containing the Image

		// Asset system integration
		ResourceID imageAssetId = INVALID_RESOURCE_ID;       // ID of ImageAsset in AssetManager
		ResourceID textureAssetId = INVALID_RESOURCE_ID;     // ID of TextureAsset in AssetManager

		// Cached properties for UI display (updated from assets)
		int width = 0;                                       // Image width
		int height = 0;                                      // Image height
		int channels = 0;                                    // Number of color channels
		GLuint textureID = 0;                                // OpenGL texture ID (cached from TextureAsset)

		ImageComponent() {
			compName = "Image";
			compCategory = "Image";
			setupBaseSchema();
		}

		virtual ~ImageComponent() {
			// Texture cleanup is handled by AssetManager
		}

		// Check if image asset is loaded
		bool IsImageLoaded() const { return imageAssetId != INVALID_RESOURCE_ID; }
		bool IsTextureReady() const { return textureAssetId != INVALID_RESOURCE_ID && textureID != 0; }

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["fileName"] = &fileName;
			properties["filePath"] = &filePath;
			return properties;
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"width", width},
				{"height", height},
				{"channels", channels},
				{"fileName", fileName},
				{"filePath", filePath},
				{"imageAssetId", imageAssetId},
				{"textureAssetId", textureAssetId}
			};
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;

			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				for (auto it = j.begin(); it != j.end(); ++it) {
					if (it.key() == compName) {
						componentData = it.value();
						break;
					}
				}
				if (componentData.empty()) {
					componentData = j;
				}
			}

			if (componentData.contains("width"))
				width = componentData["width"];
			if (componentData.contains("height"))
				height = componentData["height"];
			if (componentData.contains("channels"))
				channels = componentData["channels"];
			if (componentData.contains("fileName"))
				fileName = componentData["fileName"];
			if (componentData.contains("filePath"))
				filePath = componentData["filePath"];
			if (componentData.contains("imageAssetId"))
				imageAssetId = componentData["imageAssetId"];
			if (componentData.contains("textureAssetId"))
				textureAssetId = componentData["textureAssetId"];
		}

		ImageComponent &operator=(const ImageComponent &other) {
			if (this != &other) {
				fileName = other.fileName;
				filePath = other.filePath;
				width = other.width;
				height = other.height;
				channels = other.channels;
				imageAssetId = other.imageAssetId;
				textureAssetId = other.textureAssetId;
				textureID = other.textureID;
			}
			return *this;
		}

		// Copy constructor
		ImageComponent(const ImageComponent& other) : BaseComponent(other) {
			fileName = other.fileName;
			filePath = other.filePath;
			width = other.width;
			height = other.height;
			channels = other.channels;
			imageAssetId = other.imageAssetId;
			textureAssetId = other.textureAssetId;
			textureID = other.textureID;
			setupBaseSchema();
		}

	protected:
		void setupBaseSchema() {
			schema = {
				{"title", "Image"},
				{"type", "object"},
				{"properties", {
					{"fileName", {
						{"type", "string"},
						{"title", "File Name"}
					}},
					{"filePath", {
						{"type", "string"},
						{"title", "File Path"}
					}}
				}}
			};
		}
	};

	struct InputImageComponent : public ImageComponent {
		InputImageComponent() {
			compName = "InputImage";
			fileName = "";
			filePath = "";
			setupInputSchema();
		}

		virtual ~InputImageComponent() {
			// Asset cleanup handled by AssetManager
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;

			// Add all the properties that the schema expects to bind to
			properties["fileName"] = &fileName;
			properties["filePath"] = &filePath;
			properties["width"] = &width;
			properties["height"] = &height;
			properties["channels"] = &channels;

			return properties;
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j = ImageComponent::Serialize();
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			ImageComponent::Deserialize(j);
		}

		// Copy constructor
		InputImageComponent(const InputImageComponent& other) : ImageComponent(other) {
			compName = "InputImage";
			setupInputSchema();
		}

		InputImageComponent &operator=(const InputImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "InputImage";
				setupInputSchema();
			}
			return *this;
		}

	private:
		void setupInputSchema() {
			schema = {
				{"title", "Input Image"},
				{"type", "object"},
				{"properties", {
					{"fileName", {
						{"type", "string"},
						{"title", "Load Image"},
						{"ui:widget", "media_loader"},
						{"ui:options", {
							{"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
							{"filterName", "Image Files"},
							{"buttonText", "Load Image..."},
							{"browseTooltip", "Browse for image files (.png, .jpg, .jpeg, .bmp, .tga)"},
							{"updateProperties", {
								"fileName",
								"filePath",
								"width",
								"height",
								"channels"
							}}
						}}
					}}
				}}
			};
		}
	};

	struct OutputImageComponent : public ImageComponent {
		std::string outputDirectory = "";  // Output directory path
		std::string fileExtension = ".png";  // Selected file extension
		std::vector<std::string> supportedExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
		int selectedExtensionIndex = 0;  // Index for combo widget

		OutputImageComponent() {
			compName = "OutputImage";
			// Set default output directory
			outputDirectory = !Utils::FilePaths::outputFolderPath.empty()
				? Utils::FilePaths::outputFolderPath
				: Utils::FilePaths::defaultProjectPath;
			fileName = "AniStudio_output";
			setupOutputSchema();
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["outputDirectory"] = &outputDirectory;
			properties["fileName"] = &fileName;
			properties["selectedExtensionIndex"] = &selectedExtensionIndex;
			properties["supportedExtensions"] = &supportedExtensions;
			return properties;
		}

		// Get the full output path
		std::string GetFullOutputPath() const {
			std::string extension = fileExtension;
			if (selectedExtensionIndex >= 0 && selectedExtensionIndex < supportedExtensions.size()) {
				extension = supportedExtensions[selectedExtensionIndex];
			}

			std::string baseName = fileName;
			if (baseName.empty()) baseName = "AniStudio_output";

			std::filesystem::path outputPath = std::filesystem::path(outputDirectory) / (baseName + extension);
			return outputPath.string();
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j = ImageComponent::Serialize();
			j[compName]["outputDirectory"] = outputDirectory;
			j[compName]["fileExtension"] = fileExtension;
			j[compName]["selectedExtensionIndex"] = selectedExtensionIndex;
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			ImageComponent::Deserialize(j);

			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}

			if (componentData.contains("outputDirectory"))
				outputDirectory = componentData["outputDirectory"];
			if (componentData.contains("fileExtension"))
				fileExtension = componentData["fileExtension"];
			if (componentData.contains("selectedExtensionIndex"))
				selectedExtensionIndex = componentData["selectedExtensionIndex"];
		}

		OutputImageComponent &operator=(const OutputImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "OutputImage";
				outputDirectory = other.outputDirectory;
				fileExtension = other.fileExtension;
				selectedExtensionIndex = other.selectedExtensionIndex;
				supportedExtensions = other.supportedExtensions;
				setupOutputSchema();
			}
			return *this;
		}

		// Copy constructor
		OutputImageComponent(const OutputImageComponent& other) : ImageComponent(other) {
			compName = "OutputImage";
			outputDirectory = other.outputDirectory;
			fileExtension = other.fileExtension;
			selectedExtensionIndex = other.selectedExtensionIndex;
			supportedExtensions = other.supportedExtensions;
			setupOutputSchema();
		}

	private:
		void setupOutputSchema() {
			schema = {
				{"title", "Output Image"},
				{"type", "object"},
				{"properties", {
					{"outputDirectory", {
						{"type", "string"},
						{"title", "Output Directory"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "directory"},
							{"defaultPath", Utils::FilePaths::outputFolderPath.empty()
								? Utils::FilePaths::defaultProjectPath
								: Utils::FilePaths::outputFolderPath},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Reset"},
							{"browseTooltip", "Browse to select output directory for saving images"}
						}}
					}},
					{"fileName", {
						{"type", "string"},
						{"title", "File Name"},
						{"ui:widget", "input_text"}
					}},
					{"selectedExtensionIndex", {
						{"type", "integer"},
						{"title", "File Format"},
						{"ui:widget", "combo"},
						{"minimum", 0},
						{"maximum", 4},
						{"items", {
							{{"label", "PNG (.png)"}},
							{{"label", "JPEG (.jpg)"}},
							{{"label", "JPEG (.jpeg)"}},
							{{"label", "Bitmap (.bmp)"}},
							{{"label", "Targa (.tga)"}}
						}}
					}}
				}},
				{"propertyOrder", {"outputDirectory", "fileName", "selectedExtensionIndex"}}
			};
		}
	};

	struct ControlNetImageComponent : public ImageComponent {
		ControlNetImageComponent() {
			compName = "ControlNetImageComponent";
		}
	};

	struct MaskImageComponent : public ImageComponent {
		float value = 0.75f;
		std::string maskFilePath = "";  // Full path to mask image file

		MaskImageComponent() {
			compName = "MaskImageComponent";
			fileName = "";
			filePath = "";
			setupMaskSchema();
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["maskFilePath"] = &maskFilePath;
			properties["value"] = &value;
			return properties;
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j = ImageComponent::Serialize();
			j[compName]["value"] = value;
			j[compName]["maskFilePath"] = maskFilePath;
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			ImageComponent::Deserialize(j);

			nlohmann::json componentData;
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}

			if (componentData.contains("value"))
				value = componentData["value"];
			if (componentData.contains("maskFilePath"))
				maskFilePath = componentData["maskFilePath"];
		}

		MaskImageComponent &operator=(const MaskImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "MaskImageComponent";
				value = other.value;
				maskFilePath = other.maskFilePath;
				setupMaskSchema();
			}
			return *this;
		}

		// Copy constructor
		MaskImageComponent(const MaskImageComponent& other) : ImageComponent(other) {
			compName = "MaskImageComponent";
			value = other.value;
			maskFilePath = other.maskFilePath;
			setupMaskSchema();
		}

	private:
		void setupMaskSchema() {
			schema = {
				{"title", "Mask Image"},
				{"type", "object"},
				{"properties", {
					{"maskFilePath", {
						{"type", "string"},
						{"title", "Mask Image File"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
							{"filterName", "Image Files"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for mask image files (grayscale images)"}
						}}
					}},
					{"value", {
						{"type", "number"},
						{"title", "Mask Strength"},
						{"description", "Strength of the mask effect (0.0 to 1.0)"},
						{"ui:widget", "slider_float"},
						{"minimum", 0.0},
						{"maximum", 1.0},
						{"ui:options", {
							{"step", 0.01},
							{"format", "%.2f"}
						}}
					}}
				}},
				{"propertyOrder", {"maskFilePath", "value"}}
			};
		}
	};
} // namespace ECS