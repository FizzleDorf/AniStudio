#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace FileDialog {

    enum class FilterType {
        PATH_SELECTION,
        DIFFUSION_MODEL,
        IMAGE_FILE,
        VIDEO_FILE,
        METADATA_FILE,
        ALL_FILES
    };

    class FileFilters {
    public:
        static std::string GetFilter(FilterType type) {
            switch (type) {
            case FilterType::PATH_SELECTION:
                return "";
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
            case FilterType::VIDEO_FILE:
                return "All Video Files{.mp4,.webm},"
                    "MP4{.mp4},"
                    "WebM{.webm},"
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

        static std::vector<std::string> GetExtensions(FilterType type) {
            switch (type) {
            case FilterType::PATH_SELECTION:
                return {};
            case FilterType::DIFFUSION_MODEL:
                return { ".safetensors", ".ckpt", ".pt", ".gguf", ".bin", ".pth" };
            case FilterType::IMAGE_FILE:
                return { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".webp", ".tiff", ".gif" };
            case FilterType::VIDEO_FILE:
                return { ".mp4", ".webm" };
            case FilterType::METADATA_FILE:
                return { ".json", ".png", ".jpg", ".jpeg" };
            case FilterType::ALL_FILES:
                return {};
            default:
                return {};
            }
        }

        static std::string GetDescription(FilterType type) {
            switch (type) {
            case FilterType::PATH_SELECTION:
                return "Select Directory";
            case FilterType::DIFFUSION_MODEL:
                return "Select Diffusion Model";
            case FilterType::IMAGE_FILE:
                return "Select Image File";
            case FilterType::VIDEO_FILE:
                return "Select Video File";
            case FilterType::METADATA_FILE:
                return "Select Metadata File";
            case FilterType::ALL_FILES:
                return "Select File";
            default:
                return "Select File";
            }
        }

        static std::string GetDetailedDescription(FilterType type) {
            switch (type) {
            case FilterType::PATH_SELECTION:
                return "Choose a directory path";
            case FilterType::DIFFUSION_MODEL:
                return "Diffusion model files (.safetensors, .ckpt, .pt, .gguf, .bin, .pth)";
            case FilterType::IMAGE_FILE:
                return "Image files (.png, .jpg, .jpeg, .bmp, .tga, .webp, .tiff, .gif)";
            case FilterType::VIDEO_FILE:
                return "Video files (.mp4, .webm)";
            case FilterType::METADATA_FILE:
                return "Metadata files (.json) or images with embedded metadata (.png, .jpg, .jpeg)";
            case FilterType::ALL_FILES:
                return "All files (no filter applied)";
            default:
                return "All supported files";
            }
        }

        static bool IsValidExtension(FilterType type, const std::string& extension) {
            if (type == FilterType::ALL_FILES) return true;
            auto extensions = GetExtensions(type);
            if (extensions.empty()) return true;
            std::string lowerExt = extension;
            std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
            if (!lowerExt.empty() && lowerExt[0] != '.') {
                lowerExt = "." + lowerExt;
            }
            return std::find(extensions.begin(), extensions.end(), lowerExt) != extensions.end();
        }

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
                return GetFilter(FilterType::DIFFUSION_MODEL);
            }
        }

        struct ModelFilterInfo {
            std::string filter;
            std::string description;
        };

        static ModelFilterInfo GetModelFilterInfo(const std::string& modelType) {
            ModelFilterInfo info;
            info.filter = GetModelFilter(modelType);
            info.description = "Select " + modelType + " model file";
            if (!info.description.empty()) {
                info.description[7] = std::toupper(info.description[7]);
            }
            return info;
        }
    };

}