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
#include "WebPMetadataUtils.hpp"
#include "VideoMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include "SteganographyUtils.hpp"
#include "FileFormats.hpp"

#ifdef USE_WEBP
#include <webp/encode.h>
#include <webp/decode.h>
#endif

namespace Utils {

    class ImageUtils {
    public:
        static unsigned char* LoadImageData(const std::string& filePath, int& width, int& height, int& channels) {
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (data) {
                return data;
            }

#ifdef USE_WEBP
            std::cerr << "stb_image failed to load " << filePath << ", trying libwebp..." << std::endl;
            FILE* f = fopen(filePath.c_str(), "rb");
            if (!f) {
                std::cerr << "Cannot open file for libwebp: " << filePath << std::endl;
                return nullptr;
            }
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (size <= 0) {
                fclose(f);
                return nullptr;
            }
            uint8_t* webpData = (uint8_t*)malloc(size);
            if (!webpData) {
                fclose(f);
                return nullptr;
            }
            size_t readSize = fread(webpData, 1, size, f);
            fclose(f);
            if (readSize != (size_t)size) {
                free(webpData);
                return nullptr;
            }

            WebPBitstreamFeatures features;
            if (WebPGetFeatures(webpData, size, &features) != VP8_STATUS_OK) {
                free(webpData);
                std::cerr << "libwebp: invalid WebP file" << std::endl;
                return nullptr;
            }

            int w = features.width;
            int h = features.height;
            int has_alpha = features.has_alpha;
            uint8_t* decoded = nullptr;
            if (has_alpha) {
                decoded = WebPDecodeRGBA(webpData, size, &w, &h);
                if (decoded) channels = 4;
            }
            else {
                decoded = WebPDecodeRGB(webpData, size, &w, &h);
                if (decoded) channels = 3;
            }
            free(webpData);
            if (!decoded) {
                std::cerr << "libwebp: decoding failed" << std::endl;
                return nullptr;
            }

            width = w;
            height = h;
            unsigned char* result = (unsigned char*)malloc(width * height * channels);
            if (result) {
                memcpy(result, decoded, width * height * channels);
            }
            free(decoded);
            return result;
#else
            std::cerr << "Failed to load image: " << filePath << " - " << stbi_failure_reason() << std::endl;
            return nullptr;
#endif
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
            std::string actualPath = filePath;
            return SaveImage(actualPath, width, height, channels, data);
        }

        static bool SaveImage(std::string& filePath, int width, int height, int channels, const unsigned char* data) {
            std::filesystem::path path(filePath);
            std::filesystem::create_directories(path.parent_path());

            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            int result = 0;
            if (ext == ".png") {
                result = stbi_write_png(filePath.c_str(), width, height, channels, data, width * channels);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                result = stbi_write_jpg(filePath.c_str(), width, height, channels, data, 90);
            }
            else if (ext == ".bmp") {
                result = stbi_write_bmp(filePath.c_str(), width, height, channels, data);
            }
            else if (ext == ".tga") {
                result = stbi_write_tga(filePath.c_str(), width, height, channels, data);
            }
            else if (ext == ".webp") {
#ifdef USE_WEBP
                uint8_t* output = nullptr;
                size_t size = 0;
                if (channels == 4) {
                    size = WebPEncodeRGBA(data, width, height, width * channels, 90, &output);
                }
                else if (channels == 3) {
                    size = WebPEncodeRGB(data, width, height, width * channels, 90, &output);
                }
                else {
                    std::cerr << "Unsupported channel count for WebP: " << channels << std::endl;
                    std::string newPath = path.stem().string() + ".png";
                    std::cout << "Saving as PNG instead: " << newPath << std::endl;
                    result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                    if (result) filePath = newPath;
                    return result != 0;
                }
                if (size) {
                    FILE* f = fopen(filePath.c_str(), "wb");
                    if (f) {
                        fwrite(output, 1, size, f);
                        fclose(f);
                        result = 1;
                    }
                    WebPFree(output);
                }
                else {
                    std::cerr << "WebP encoding failed, falling back to PNG" << std::endl;
                    std::string newPath = path.stem().string() + ".png";
                    std::cout << "Saving as PNG: " << newPath << std::endl;
                    result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                    if (result) filePath = newPath;
                }
#else
                std::string newPath = path.stem().string() + ".png";
                std::cout << "WebP not supported, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                if (result) filePath = newPath;
#endif
            }
            else if (ext == ".hdr" || ext == ".pic" || ext == ".pnm") {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "Format " << ext << " not supported for writing, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                if (result) filePath = newPath;
            }
            else if (ext == ".tiff") {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "TIFF writing not supported, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                if (result) filePath = newPath;
            }
            else if (ext == ".gif") {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "GIF writing not supported, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                if (result) filePath = newPath;
            }
            else {
                std::string newPath = path.stem().string() + ".png";
                std::cout << "Unknown format, saving as PNG: " << newPath << std::endl;
                result = stbi_write_png(newPath.c_str(), width, height, channels, data, width * channels);
                if (result) filePath = newPath;
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

            const auto& formats = FileFormats::GetAllFormats();
            auto it = formats.find(ext);
            bool supportsMetadata = (it != formats.end()) ? it->second.supportsMetadata : false;
            bool supportsStealth = (it != formats.end()) ? it->second.supportsStealth : false;

            if (!supportsMetadata || (useStealth && !supportsStealth)) {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
            }

            if (ext == ".png") {
                return PngMetadata::WriteMetadataToPNG(imagePath, metadata, useStealth);
            }
            else if (ext == ".jpg" || ext == ".jpeg") {
                return JpegMetadata::WriteMetadataToJPEG(imagePath, metadata);
            }
            else if (ext == ".webp") {
                return WebPMetadata::WriteMetadataToWebP(imagePath, metadata);
            }
            else if (ext == ".tiff") {
#ifdef USE_EXIV2
                return MetadataUtils::WriteMetadataToTIFF(imagePath, metadata);
#else
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
#endif
            }
            else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" || ext == ".mov" || ext == ".flv" ||
                ext == ".m4v" || ext == ".3gp" || ext == ".ogv" || ext == ".ts" || ext == ".wmv" || ext == ".mpg" || ext == ".mpeg") {
                return VideoMetadataUtils::WriteMetadataToVideo(imagePath, metadata, forceSidecar);
            }
            else if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".m4a") {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
            }
            else {
                std::string jsonPath = imagePath + ".json";
                return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
            }
        }

        static nlohmann::json ReadMetadataFromImage(const std::string& imagePath) {
            std::string ext = std::filesystem::path(imagePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            nlohmann::json result;

            try {
                if (ext == ".png") {
                    result = PngMetadata::ReadMetadataFromPNG(imagePath);
                }
                else if (ext == ".jpg" || ext == ".jpeg") {
                    result = JpegMetadata::ReadMetadataFromJPEG(imagePath);
                }
                else if (ext == ".webp") {
                    result = WebPMetadata::ReadMetadataFromWebP(imagePath);
                }
                else if (ext == ".tiff") {
#ifdef USE_EXIV2
                    result = MetadataUtils::ReadMetadataFromTIFF(imagePath);
#endif
                }
                else if (ext == ".mp4" || ext == ".webm" || ext == ".mkv" || ext == ".avi" || ext == ".mov" || ext == ".flv" ||
                    ext == ".m4v" || ext == ".3gp" || ext == ".ogv" || ext == ".ts" || ext == ".wmv" || ext == ".mpg" || ext == ".mpeg") {
                    result = VideoMetadataUtils::ReadMetadataFromVideo(imagePath);
                }
                else if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".m4a") {
                }
            }
            catch (...) {
                result = nlohmann::json();
            }

            std::string jsonPath = imagePath + ".json";
            if (std::filesystem::exists(jsonPath)) {
                nlohmann::json sidecar = MetadataUtils::LoadMetadataFromJson(jsonPath);
                if (!sidecar.empty()) {
                    for (auto it = sidecar.begin(); it != sidecar.end(); ++it) {
                        result[it.key()] = it.value();
                    }
                }
            }

            result = MetadataUtils::NormalizeAniStudioMetadata(result);
            return result;
        }

        static bool HasExifMetadata(const std::string& filePath) {
            nlohmann::json meta;
            try {
                meta = ReadMetadataFromImage(filePath);
            }
            catch (...) {
                return false;
            }
            if (meta.is_null() || meta.empty()) return false;

            if (meta.contains("components") && meta["components"].is_array()) {
                for (const auto& comp : meta["components"]) {
                    if (comp.is_object() && !comp.empty()) {
                        for (auto it = comp.begin(); it != comp.end(); ++it) {
                            if (!it.value().is_null() && !it.value().empty()) {
                                return true;
                            }
                        }
                    }
                }
            }

            for (auto it = meta.begin(); it != meta.end(); ++it) {
                if (it.key() != "components" && it.value().is_object() && !it.value().empty()) {
                    return true;
                }
                if (it.value().is_string() && !it.value().get<std::string>().empty()) {
                    return true;
                }
            }
            return false;
        }

        static bool HasLSBMetadata(const std::string& filePath) {
            int width, height, channels;
            unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
            if (!data) {
                return false;
            }

            std::vector<unsigned char> extracted = SteganographyUtils::ExtractFromAlpha(data, width, height);
            stbi_image_free(data);

            return !extracted.empty();
        }

        static int GetMetadataStatus(const std::string& filePath) {
            nlohmann::json meta;
            try {
                meta = ReadMetadataFromImage(filePath);
            }
            catch (...) {
                return 0;
            }
            if (meta.is_null() || meta.empty()) return 0;

            if (meta.contains("components") && meta["components"].is_array()) {
                for (const auto& comp : meta["components"]) {
                    if (comp.is_object() && !comp.empty()) {
                        for (auto it = comp.begin(); it != comp.end(); ++it) {
                            if (!it.value().is_null() && !it.value().empty()) {
                                return 1;
                            }
                        }
                    }
                }
            }

            if (meta.contains("dataType") && meta["dataType"] == "entity" && meta.contains("data")) {
                return 1;
            }

            for (auto it = meta.begin(); it != meta.end(); ++it) {
                if (it.value().is_object() && !it.value().empty()) {
                    return 2;
                }
                if (it.value().is_string() && !it.value().get<std::string>().empty()) {
                    return 2;
                }
            }
            return 0;
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

}