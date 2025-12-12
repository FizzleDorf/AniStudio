#pragma once

#include "BaseComponent.hpp"
#include "FilePaths.hpp"
#include "OpenGLWrapper.hpp"
#include <string>
#include <stb_image.h>
#include <memory>

namespace ECS {
	struct ImageComponent : public BaseComponent {
		std::string fileName = "AniStudio";                  // Default file name
		std::string filePath =
			!Utils::FilePaths::outputFolderPath.empty()
			? Utils::FilePaths::outputFolderPath			 // Output folder if one is found
			: Utils::FilePaths::defaultProjectPath;			 // Directory containing the Image
		unsigned char *imageData = nullptr;                  // Pointer to image data - DO NOT FREE in destructor for base class
		int width = 0;                                       // Image width
		int height = 0;                                      // Image height
		int channels = 0;                                    // Number of color channels
		GLuint textureID = 0;                                // OpenGL texture ID

		ImageComponent() {
			compName = "Image";
			compCategory = "Image";
			setupBaseSchema();
		}

		virtual ~ImageComponent() {
			// Base ImageComponent doesn't own imageData - managed by ImageSystem
			// Only cleanup texture
			if (textureID != 0) {
				glDeleteTextures(1, &textureID);
				textureID = 0;
			}
		}

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
				{"filePath", filePath}
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
		}

		ImageComponent &operator=(const ImageComponent &other) {
			if (this != &other) {
				fileName = other.fileName;
				filePath = other.filePath;
				width = other.width;
				height = other.height;
				channels = other.channels;
				// Don't copy imageData pointer - each component manages its own
				// Don't copy textureID - each component needs its own texture
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
			imageData = nullptr; // Don't copy raw pointer
			textureID = 0; // Don't copy texture ID
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
		std::shared_ptr<unsigned char[]> ownedImageData; // Smart pointer for owned data

		InputImageComponent() {
			compName = "InputImage";
			compCategory = "Image";
			fileName = "";
			filePath = "";
			width = 0;
			height = 0;
			channels = 0;
			setupInputSchema();
		}

		virtual ~InputImageComponent() {
			// Cleanup happens automatically via shared_ptr
			// Texture cleanup handled by base class
		}

		// Get property map for UI rendering - MUST INCLUDE ALL PROPERTIES
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["fileName"] = &fileName;
			properties["filePath"] = &filePath;
			properties["width"] = &width;
			properties["height"] = &height;
			properties["channels"] = &channels;
			return properties;
		}

		// Serialize the component to JSON - MUST INCLUDE ALL PROPERTIES
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j;
			j["compName"] = compName;
			j[compName] = {
				{"fileName", fileName},
				{"filePath", filePath},
				{"width", width},
				{"height", height},
				{"channels", channels}
			};
			return j;
		}

		// Deserialize the component from JSON
		virtual void Deserialize(const nlohmann::json& j) override {
			BaseComponent::Deserialize(j);

			nlohmann::json componentData;

			// Handle different JSON structures
			if (j.contains(compName)) {
				componentData = j.at(compName);
			}
			else {
				// Try to find component data by name
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

			// Load ALL properties from JSON
			if (componentData.contains("fileName"))
				fileName = componentData["fileName"];
			if (componentData.contains("filePath"))
				filePath = componentData["filePath"];
			if (componentData.contains("width"))
				width = componentData["width"];
			if (componentData.contains("height"))
				height = componentData["height"];
			if (componentData.contains("channels"))
				channels = componentData["channels"];
		}

		void SetImageData(unsigned char* data, int w, int h, int ch) {
			if (data && w > 0 && h > 0 && ch > 0) {
				// Calculate data size
				size_t dataSize = w * h * ch;

				// Create shared_ptr with custom deleter
				ownedImageData = std::shared_ptr<unsigned char[]>(
					data,
					[](unsigned char* ptr) {
					if (ptr) {
						stbi_image_free(ptr);
					}
				}
				);

				// Set the raw pointer for backward compatibility
				imageData = ownedImageData.get();
				width = w;
				height = h;
				channels = ch;
			}
			else {
				ClearImageData();
			}
		}

		void ClearImageData() {
			ownedImageData.reset();
			imageData = nullptr;
			width = 0;
			height = 0;
			channels = 0;
		}

		// Copy constructor
		InputImageComponent(const InputImageComponent& other) : ImageComponent(other) {
			compName = "InputImage";
			setupInputSchema();

			// Copy scalar values
			fileName = other.fileName;
			filePath = other.filePath;
			width = other.width;
			height = other.height;
			channels = other.channels;

			// Deep copy image data if it exists
			if (other.ownedImageData && other.width > 0 && other.height > 0 && other.channels > 0) {
				// Create a deep copy of the image data
				size_t dataSize = other.width * other.height * other.channels;
				unsigned char* newData = static_cast<unsigned char*>(malloc(dataSize));
				if (newData) {
					memcpy(newData, other.ownedImageData.get(), dataSize);
					SetImageData(newData, other.width, other.height, other.channels);
				}
			}
		}

		InputImageComponent &operator=(const InputImageComponent &other) {
			if (this != &other) {
				// Call base assignment
				ImageComponent::operator=(other);
				compName = "InputImage";

				// Copy scalar values
				fileName = other.fileName;
				filePath = other.filePath;
				width = other.width;
				height = other.height;
				channels = other.channels;

				// Deep copy image data if it exists
				if (other.ownedImageData && other.width > 0 && other.height > 0 && other.channels > 0) {
					size_t dataSize = other.width * other.height * other.channels;
					unsigned char* newData = static_cast<unsigned char*>(malloc(dataSize));
					if (newData) {
						memcpy(newData, other.ownedImageData.get(), dataSize);
						SetImageData(newData, other.width, other.height, other.channels);
					}
				}
				else {
					ClearImageData();
				}
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
					{"filePath", {
						{"type", "string"},
						{"title", "Input Image File"},
						{"ui:widget", "file_selector"},
						{"ui:options", {
							{"mode", "file"},
							{"filters", ".png,.jpg,.jpeg,.bmp,.tga"},
							{"filterName", "Image Files"},
							{"buttonText", "Browse..."},
							{"resetButtonText", "Clear"},
							{"browseTooltip", "Browse for image files (.png, .jpg, .jpeg, .bmp, .tga)"}
						}}
					}}
				}},
				{"propertyOrder", {"filePath", "fileName", "width", "height", "channels"}}
			};
		}
	};

	struct OutputImageComponent : public ImageComponent {
		std::string fileExtension = ".png";  // Selected file extension
		std::vector<std::string> supportedExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };
		int selectedExtensionIndex = 0;  // Index for combo widget

		OutputImageComponent() {
			compName = "OutputImage";
			compCategory = "Image";
			// Set default output directory to filePath 
			filePath = !Utils::FilePaths::outputFolderPath.empty()
				? Utils::FilePaths::outputFolderPath
				: Utils::FilePaths::defaultProjectPath;
			fileName = "AniStudio";
			setupOutputSchema();
		}

		// Get property map for UI rendering
		virtual std::unordered_map<std::string, UISchema::PropertyVariant> GetPropertyMap() override {
			std::unordered_map<std::string, UISchema::PropertyVariant> properties;
			properties["fileName"] = &fileName;
			properties["filePath"] = &filePath;  // ADD FILEPATH TO PROPERTIES!
			properties["selectedExtensionIndex"] = &selectedExtensionIndex;
			properties["supportedExtensions"] = &supportedExtensions;
			return properties;
		}

		// Get the full output path - SIMPLIFIED
		std::string GetFullOutputPath() const {
			std::string extension = fileExtension;
			if (selectedExtensionIndex >= 0 && selectedExtensionIndex < supportedExtensions.size()) {
				extension = supportedExtensions[selectedExtensionIndex];
			}

			std::string baseName = fileName;
			if (baseName.empty()) baseName = "AniStudio";

			// JUST USE FILEPATH - NO MORE outputDirectory BULLSHIT
			std::string outputDir = filePath;
			if (outputDir.empty()) {
				outputDir = !Utils::FilePaths::outputFolderPath.empty()
					? Utils::FilePaths::outputFolderPath
					: Utils::FilePaths::defaultProjectPath;
			}

			std::filesystem::path outputPath = std::filesystem::path(outputDir) / (baseName + extension);
			return outputPath.string();
		}

		// Serialize the component to JSON
		virtual nlohmann::json Serialize() const override {
			nlohmann::json j = ImageComponent::Serialize();
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

			if (componentData.contains("fileExtension"))
				fileExtension = componentData["fileExtension"];
			if (componentData.contains("selectedExtensionIndex"))
				selectedExtensionIndex = componentData["selectedExtensionIndex"];
		}

		OutputImageComponent &operator=(const OutputImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "OutputImage";
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
					{"filePath", {  // CHANGE FROM outputDirectory TO filePath!
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
				{"propertyOrder", {"filePath", "fileName", "selectedExtensionIndex"}}  // CHANGED HERE TOO
			};
		}
	};

	// TODO: these are just placeholders until the node execution is implemented

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