#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "FilePathService.hpp"

namespace FileDialog {

	enum class FilterType {
		PATH_SELECTION,      // For selecting directories/paths
		DIFFUSION_MODEL,     // For diffusion model files
		IMAGE_FILE,          // For image files
		METADATA_FILE,       // For JSON/metadata files
		ALL_FILES           // Show all files
	};

	class FileFilters {
	public:
		// Get filter string for ImGuiFileDialog with separate filters and All Files option
		static std::string GetFilter(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return ""; // No filter for directory selection

			case FilterType::DIFFUSION_MODEL:
				return "All Model Files{.safetensors,.ckpt,.pt,.gguf,.bin,.pth},"
					"SafeTensors{.safetensors},"
					"Checkpoint{.ckpt},"
					"PyTorch{.pt,.pth},"
					"GGUF{.gguf},"
					"Binary{.bin},"
					"All Files{.*}";

			case FilterType::IMAGE_FILE:
				return "All Image Files{.png,.jpg,.jpeg,.bmp,.tga,.webp,.tiff,.gif},"
					"PNG{.png},"
					"JPEG{.jpg,.jpeg},"
					"Bitmap{.bmp},"
					"Targa{.tga},"
					"WebP{.webp},"
					"TIFF{.tiff},"
					"GIF{.gif},"
					"All Files{.*}";

			case FilterType::METADATA_FILE:
				return "All Metadata Files{.json,.png,.jpg,.jpeg},"
					"JSON{.json},"
					"PNG with Metadata{.png},"
					"JPEG with Metadata{.jpg,.jpeg},"
					"All Files{.*}";

			case FilterType::ALL_FILES:
				return "All Files{.*}";

			default:
				return "All Files{.*}";
			}
		}

		// Get simple extension list (for validation)
		static std::vector<std::string> GetExtensions(FilterType type) {
			switch (type) {
			case FilterType::PATH_SELECTION:
				return {}; // No extensions for directory selection

			case FilterType::DIFFUSION_MODEL:
				return { ".safetensors", ".ckpt", ".pt", ".gguf", ".bin", ".pth" };

			case FilterType::IMAGE_FILE:
				return { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".webp", ".tiff", ".gif" };

			case FilterType::METADATA_FILE:
				return { ".json", ".png", ".jpg", ".jpeg" };

			case FilterType::ALL_FILES:
				return {}; // All files allowed

			default:
				return {};
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

			case FilterType::ALL_FILES:
				return "Select File";

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

			case FilterType::ALL_FILES:
				return "All files (no filter applied)";

			default:
				return "All supported files";
			}
		}

		// Check if a file extension is valid for the given filter type
		static bool IsValidExtension(FilterType type, const std::string& extension) {
			if (type == FilterType::ALL_FILES) return true; // All files allowed

			auto extensions = GetExtensions(type);
			if (extensions.empty()) return true; // No filter means all files allowed

			// Convert to lowercase for comparison
			std::string lowerExt = extension;
			std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

			// Ensure extension starts with dot
			if (!lowerExt.empty() && lowerExt[0] != '.') {
				lowerExt = "." + lowerExt;
			}

			return std::find(extensions.begin(), extensions.end(), lowerExt) != extensions.end();
		}

		// Get recommended default paths for each filter type
		static std::string GetDefaultPath(FilterType type) {			
			switch (type) {
			case FilterType::PATH_SELECTION:
				return Utils::FilePathService::GetPath("DefaultProject");

			case FilterType::DIFFUSION_MODEL:
				return Utils::FilePathService::GetPath("Checkpoint");

			case FilterType::IMAGE_FILE:
				return Utils::FilePathService::GetPath("DefaultProject");

			case FilterType::METADATA_FILE:
				return Utils::FilePathService::GetPath("DefaultProject");

			case FilterType::ALL_FILES:
				return Utils::FilePathService::GetPath("DefaultProject");

			default:
				return Utils::FilePathService::GetPath("DefaultProject");
			}
		}

		// Specialized getters for specific model types
		static std::string GetModelPath(const std::string& modelType) {
			
			static std::unordered_map<std::string, std::string> modelPaths = {
				{"checkpoint", Utils::FilePathService::GetPath("Checkpoint")},
				{"unet", Utils::FilePathService::GetPath("Unet")},
				{"vae", Utils::FilePathService::GetPath("Vae")},
				{"clip", Utils::FilePathService::GetPath("Encoder")},
				{"t5", Utils::FilePathService::GetPath("Encoder")},
				{"lora", Utils::FilePathService::GetPath("Lora")},
				{"controlnet", Utils::FilePathService::GetPath("ControlNet")},
				{"embedding", Utils::FilePathService::GetPath("Embed")},
				{"upscale", Utils::FilePathService::GetPath("Upscale")}
			};

			auto it = modelPaths.find(modelType);
			return (it != modelPaths.end()) ? it->second : Utils::FilePathService::GetPath("Checkpoint");
		}

		// Get filter for specific model type with separate categories
		static std::string GetModelFilter(const std::string& modelType) {
			std::string lowerType = modelType;
			std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

			if (lowerType == "checkpoint" || lowerType == "model") {
				return "All Model Files{.safetensors,.ckpt,.pt,.bin,.pth},"
					"SafeTensors{.safetensors},"
					"Checkpoint{.ckpt},"
					"PyTorch{.pt,.pth},"
					"Binary{.bin},"
					"All Files{.*}";
			}
			else if (lowerType == "vae" || lowerType == "clip" || lowerType == "t5" || lowerType == "unet") {
				return "All Model Files{.safetensors,.pt,.bin,.pth},"
					"SafeTensors{.safetensors},"
					"PyTorch{.pt,.pth},"
					"Binary{.bin},"
					"All Files{.*}";
			}
			else if (lowerType == "lora") {
				return "All LoRA Files{.safetensors,.ckpt,.pt,.bin},"
					"SafeTensors{.safetensors},"
					"Checkpoint{.ckpt},"
					"PyTorch{.pt},"
					"Binary{.bin},"
					"All Files{.*}";
			}
			else if (lowerType == "upscale" || lowerType == "esrgan") {
				return "All Upscale Models{.safetensors,.pt,.pth,.bin},"
					"SafeTensors{.safetensors},"
					"PyTorch{.pt,.pth},"
					"Binary{.bin},"
					"All Files{.*}";
			}
			else {
				// Default to general model filter
				return GetFilter(FilterType::DIFFUSION_MODEL);
			}
		}

		// Convenience method to get both filter and default path for model types
		struct ModelFilterInfo {
			std::string filter;
			std::string defaultPath;
			std::string description;
		};

		static ModelFilterInfo GetModelFilterInfo(const std::string& modelType) {			
			ModelFilterInfo info;
			info.filter = GetModelFilter(modelType);
			info.defaultPath = GetModelPath(modelType);
			info.description = "Select " + modelType + " model file";

			// Capitalize first letter for description
			if (!info.description.empty()) {
				info.description[7] = std::toupper(info.description[7]); // Position of modelType in "Select "
			}

			return info;
		}
	};

} // namespace FileDialog