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

#include <string>
#include <unordered_map>

namespace FileDialog {

	enum class FilterType {
		PATH_SELECTION,      // For selecting directories/paths
		DIFFUSION_MODEL,     // For diffusion model files
		IMAGE_FILE,          // For image files
		METADATA_FILE        // For JSON/metadata files
	};

	class FileFilters {
	public:
		// Get filter string for ImGuiFileDialog
		static std::string GetFilter(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return ""; // No filter for directory selection

			case FilterType::DIFFUSION_MODEL:
				return ".safetensors,.ckpt,.pt,.gguf,.bin,.pth";

			case FilterType::IMAGE_FILE:
				return ".png,.jpg,.jpeg,.bmp,.tga,.webp,.tiff,.gif";

			case FilterType::METADATA_FILE:
				return ".json,.png,.jpg,.jpeg"; // JSON files + images with metadata

			default:
				return "";
			}
		}

		// Get human-readable description for dialog title
		static std::string GetDescription(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return "Select Directory";

			case FilterType::DIFFUSION_MODEL:
				return "Select Diffusion Model";

			case FilterType::IMAGE_FILE:
				return "Select Image File";

			case FilterType::METADATA_FILE:
				return "Select Metadata File";

			default:
				return "Select File";
			}
		}

		// Get detailed filter descriptions for UI display
		static std::string GetDetailedDescription(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return "Choose a directory path";

			case FilterType::DIFFUSION_MODEL:
				return "Diffusion model files (.safetensors, .ckpt, .pt, .gguf, .bin, .pth)";

			case FilterType::IMAGE_FILE:
				return "Image files (.png, .jpg, .jpeg, .bmp, .tga, .webp, .tiff, .gif)";

			case FilterType::METADATA_FILE:
				return "Metadata files (.json) or images with embedded metadata (.png, .jpg, .jpeg)";

			default:
				return "All supported files";
			}
		}

		// Check if a file extension is valid for the given filter type
		static bool IsValidExtension(FilterType type, const std::string& extension) {
			std::string filter = GetFilter(type);
			if (filter.empty()) return true; // No filter means all files allowed

			// Convert to lowercase for comparison
			std::string lowerExt = extension;
			std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

			return filter.find(lowerExt) != std::string::npos;
		}

		// Get recommended default paths for each filter type
		static std::string GetDefaultPath(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return Utils::FilePaths::defaultProjectPath;

			case FilterType::DIFFUSION_MODEL:
				return Utils::FilePaths::checkpointDir;

			case FilterType::IMAGE_FILE:
				return Utils::FilePaths::defaultProjectPath;

			case FilterType::METADATA_FILE:
				return Utils::FilePaths::defaultProjectPath;

			default:
				return Utils::FilePaths::defaultProjectPath;
			}
		}

		// Specialized getters for specific model types
		static std::string GetModelPath(const std::string& modelType) {
			static std::unordered_map<std::string, std::string> modelPaths = {
				{"checkpoint", Utils::FilePaths::checkpointDir},
				{"unet", Utils::FilePaths::unetDir},
				{"vae", Utils::FilePaths::vaeDir},
				{"clip", Utils::FilePaths::encoderDir},
				{"t5", Utils::FilePaths::encoderDir},
				{"lora", Utils::FilePaths::loraDir},
				{"controlnet", Utils::FilePaths::controlnetDir},
				{"embedding", Utils::FilePaths::embedDir},
				{"upscale", Utils::FilePaths::upscaleDir}
			};

			auto it = modelPaths.find(modelType);
			return (it != modelPaths.end()) ? it->second : Utils::FilePaths::checkpointDir;
		}
	};

} // namespace FileDialog