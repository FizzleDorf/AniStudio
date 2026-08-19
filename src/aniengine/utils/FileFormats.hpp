#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace FileFormats {

    struct FormatInfo {
        std::string extension;   // with leading dot, e.g. ".png"
        std::string description;
        bool isImage = true;
        bool isVideo = false;
        bool isAudio = false;
        bool supportsMetadata = false;
        bool supportsStealth = false;
        bool canWrite = true;
        bool canRead = true;
    };

    inline const std::unordered_map<std::string, FormatInfo>& GetAllFormats() {
        static const std::unordered_map<std::string, FormatInfo> formats = {
            { ".png",   { ".png",   "PNG Image", true, false, false, true, true, true, true } },
            { ".jpg",   { ".jpg",   "JPEG Image", true, false, false, true, false, true, true } },
            { ".jpeg",  { ".jpeg",  "JPEG Image", true, false, false, true, false, true, true } },
            { ".bmp",   { ".bmp",   "Bitmap Image", true, false, false, false, false, true, true } },
            { ".tga",   { ".tga",   "Targa Image", true, false, false, false, false, true, true } },
            { ".webp",  { ".webp",  "WebP Image", true, false, false, true, true, true, true } },
            { ".tiff",  { ".tiff",  "TIFF Image", true, false, false, true, false, true, true } },
            { ".gif",   { ".gif",   "GIF Image", true, false, false, false, false, true, true } },
            { ".hdr",   { ".hdr",   "HDR Image", true, false, false, false, false, true, true } },
            { ".pic",   { ".pic",   "PIC Image", true, false, false, false, false, true, true } },
            { ".pnm",   { ".pnm",   "PNM Image", true, false, false, false, false, true, true } },
            { ".mp4",   { ".mp4",   "MP4 Video", false, true, false, true, false, true, true } },
            { ".webm",  { ".webm",  "WebM Video", false, true, false, true, false, true, true } },
            { ".mkv",   { ".mkv",   "Matroska Video", false, true, false, true, false, true, true } },
            { ".avi",   { ".avi",   "AVI Video", false, true, false, true, false, true, true } },
            { ".mov",   { ".mov",   "QuickTime Video", false, true, false, true, false, true, true } },
            { ".flv",   { ".flv",   "FLV Video", false, true, false, false, false, true, true } },
            { ".wmv",   { ".wmv",   "WMV Video", false, true, false, false, false, true, true } },
            { ".mpg",   { ".mpg",   "MPEG Video", false, true, false, false, false, true, true } },
            { ".mpeg",  { ".mpeg",  "MPEG Video", false, true, false, false, false, true, true } },
            { ".3gp",   { ".3gp",   "3GP Video", false, true, false, false, false, true, true } },
            { ".ogv",   { ".ogv",   "OGV Video", false, true, false, false, false, true, true } },
            { ".ogg",   { ".ogg",   "OGG Video", false, true, false, false, false, true, true } },
            { ".ts",    { ".ts",    "Transport Stream", false, true, false, false, false, true, true } },
            { ".m4v",   { ".m4v",   "M4V Video", false, true, false, true, false, true, true } },
            { ".wav",   { ".wav",   "WAV Audio", false, false, true, false, false, true, true } },
            { ".mp3",   { ".mp3",   "MP3 Audio", false, false, true, false, false, true, true } },
            { ".flac",  { ".flac",  "FLAC Audio", false, false, true, false, false, true, true } },
            { ".aac",   { ".aac",   "AAC Audio", false, false, true, false, false, true, true } },
            { ".ogg",   { ".ogg",   "OGG Audio", false, false, true, false, false, true, true } },
            { ".m4a",   { ".m4a",   "M4A Audio", false, false, true, false, false, true, true } },
            { ".opus",  { ".opus",  "Opus Audio", false, false, true, false, false, true, true } }
        };
        return formats;
    }

    inline std::vector<std::string> GetImageExtensions() {
        std::vector<std::string> exts;
        for (const auto& pair : GetAllFormats()) {
            if (pair.second.isImage) exts.push_back(pair.first);
        }
        return exts;
    }

    inline std::vector<std::string> GetVideoExtensions() {
        std::vector<std::string> exts;
        for (const auto& pair : GetAllFormats()) {
            if (pair.second.isVideo) exts.push_back(pair.first);
        }
        return exts;
    }

    inline std::vector<std::string> GetAudioExtensions() {
        std::vector<std::string> exts;
        for (const auto& pair : GetAllFormats()) {
            if (pair.second.isAudio) exts.push_back(pair.first);
        }
        return exts;
    }

    inline nlohmann::json GetComboItemsJson(const std::vector<std::string>& extensions) {
        nlohmann::json items = nlohmann::json::array();
        auto& formats = GetAllFormats();
        for (const auto& ext : extensions) {
            auto it = formats.find(ext);
            if (it != formats.end()) {
                items.push_back({
                    {"label", it->second.description + " (" + ext + ")"},
                    {"value", ext}
                    });
            }
        }
        return items;
    }

} // namespace FileFormats