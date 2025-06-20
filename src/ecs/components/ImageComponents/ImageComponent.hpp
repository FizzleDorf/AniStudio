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
#include <GL/glew.h>
#include <string>
#include <stb_image.h>
#include <memory>

namespace ECS {
	struct ImageComponent : public BaseComponent {
		std::string fileName = "AniStudio";                  // Default file name
		std::string filePath = Utils::FilePaths::defaultProjectPath; // Directory containing the Image
		unsigned char *imageData = nullptr;                  // Pointer to image data - DO NOT FREE in destructor for base class
		int width = 0;                                       // Image width
		int height = 0;                                      // Image height
		int channels = 0;                                    // Number of color channels
		GLuint textureID = 0;                                // OpenGL texture ID

		ImageComponent() {
			compName = "Image";
		}

		virtual ~ImageComponent() {
			// Base ImageComponent doesn't own imageData - managed by ImageSystem
			// Only cleanup texture
			if (textureID != 0) {
				glDeleteTextures(1, &textureID);
				textureID = 0;
			}
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
		}
	};

	struct InputImageComponent : public ImageComponent {
		std::shared_ptr<unsigned char[]> ownedImageData; // Smart pointer for owned data

		InputImageComponent() {
			compName = "InputImage";
			fileName = "";
			filePath = "";
		}

		virtual ~InputImageComponent() {
			// Cleanup happens automatically via shared_ptr
			// Texture cleanup handled by base class
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
			}
			return *this;
		}
	};

	struct OutputImageComponent : public ImageComponent {
		OutputImageComponent() {
			compName = "OutputImage";
		}

		OutputImageComponent &operator=(const OutputImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "OutputImage";
			}
			return *this;
		}

		// Copy constructor
		OutputImageComponent(const OutputImageComponent& other) : ImageComponent(other) {
			compName = "OutputImage";
		}
	};

	struct ControlNetImageComponent : public ImageComponent {
		ControlNetImageComponent() {
			compName = "ControlNetImageComponent";
		}
	};

	struct MaskImageComponent : public ImageComponent {
		float value = 0.75f;

		MaskImageComponent() {
			compName = "MaskImageComponent";
			fileName = "";
			filePath = "";
		}

		MaskImageComponent &operator=(const MaskImageComponent &other) {
			if (this != &other) {
				ImageComponent::operator=(other);
				compName = "MaskImageComponent";
				value = other.value;
			}
			return *this;
		}

		// Copy constructor
		MaskImageComponent(const MaskImageComponent& other) : ImageComponent(other) {
			compName = "MaskImageComponent";
			value = other.value;
		}
	};
} // namespace ECS