#pragma once

#include <string>
#include <stb_image.h>
#include <stb_image_write.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "PngMetadataUtils.hpp"
#include "JpegMetadataUtils.hpp"
#include "MetadataUtils.hpp"

namespace Utils {

    class ImageUtils {
    public:
        static unsigned char* LoadImageData(const std::string& filePath, int& width, int& height, int& channels) {
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (!data) {
                std::cerr << "Failed to load image: " << filePath << " - " << stbi_failure_reason() << std::endl;
                return nullptr;
            }
            return data;
        }

        static void FreeImageData(unsigned char* data) {
            if (data) stbi_image_free(data);
        }

        static unsigned char* CopyImageData(const unsigned char* src, int width, int height, int channels) {
            if (!src) return nullptr;
            size_t dataSize = width * height * channels;
            unsigned char* copy = static_cast<unsigned char*>(malloc(dataSize));
            if (copy) memcpy(copy, src, dataSize);
            return copy;
        }

        static bool SaveImage(const std::string& filePath, int width, int height, int channels, const unsigned char* data) {
            std::filesystem::path path(filePath);
            std::filesystem::create_directories(path.parent_path());

            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            int result = 0;
            if (ext == ".png")
                result = stbi_write_png(filePath.c_str(), width, height, channels, data, width * channels);
            else if (ext == ".jpg" || ext == ".jpeg")
                result = stbi_write_jpg(filePath.c_str(), width, height, channels, data, 90);
            else if (ext == ".bmp")
                result = stbi_write_bmp(filePath.c_str(), width, height, channels, data);
            else if (ext == ".tga")
                result = stbi_write_tga(filePath.c_str(), width, height, channels, data);
            else if (ext == ".hdr" || ext == ".pic" || ext == ".pnm") {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "Format " << ext << " not supported for writing, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
            }
            else {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "Unknown format, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
            }
            return result != 0;
        }

        static bool WriteMetadataToImage(const std::string& imagePath, const nlohmann::json& metadata,
            bool useStealth = false, bool forceSidecar = false) {
            if (forceSidecar) {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
            }

            std::string ext = std::filesystem::path(imagePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png") {
                return PngMetadata::WriteMetadataToPNG(imagePath, metadata, useStealth);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                return JpegMetadata::WriteMetadataToJPEG(imagePath, metadata);
            }
            else {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
            }
        }

        static nlohmann::json ReadMetadataFromImage(const std::string& imagePath) {
            std::string ext = std::filesystem::path(imagePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".png") {
                return PngMetadata::ReadMetadataFromPNG(imagePath);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                return JpegMetadata::ReadMetadataFromJPEG(imagePath);
            }
            else {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::LoadMetadataFromJson(jsonPath);
            }
        }

        static std::string CreateUniqueFilenameIncremental(const std::string& baseName, const std::string& directory, const std::string& extension) {
            std::filesystem::path dirPath(directory);
            std::filesystem::path basePath(baseName);
            if (!std::filesystem::exists(dirPath)) std::filesystem::create_directories(dirPath);

            std::string filename = basePath.stem().string();
            std::string normalizedExt = extension;
            if (!extension.empty() && extension[0] != '.') normalizedExt = "." + extension;
            if (extension.empty()) normalizedExt = ".png";

            int highestIndex = 0;
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                if (entry.path().extension() == normalizedExt) {
                    std::string entryName = entry.path().stem().string();
                    if (entryName.find(filename + "_") == 0) {
                        try {
                            size_t underscorePos = entryName.find('_');
                            if (underscorePos != std::string::npos) {
                                int index = std::stoi(entryName.substr(underscorePos + 1));
                                if (index > highestIndex) highestIndex = index;
                            }
                        }
                        catch (...) {}
                    }
                }
            }
            highestIndex++;
            std::ostringstream formattedIndex;
            formattedIndex << std::setw(5) << std::setfill('0') << highestIndex;
            std::filesystem::path newFilePath = dirPath / (filename + "_" + formattedIndex.str() + normalizedExt);
            return newFilePath.string();
        }

        static std::string CreateUniqueFilenameDatetime(const std::string& baseName, const std::string& directory, const std::string& extension) {
            std::filesystem::path dirPath(directory);
            std::filesystem::path basePath(baseName);
            if (!std::filesystem::exists(dirPath)) std::filesystem::create_directories(dirPath);

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");

            std::string normalizedExt = extension;
            if (!extension.empty() && extension[0] != '.') normalizedExt = "." + extension;
            if (extension.empty()) normalizedExt = ".png";

            std::string filename = basePath.stem().string() + "_" + ss.str() + normalizedExt;
            return (dirPath / filename).string();
        }
    };

} // namespace Utils