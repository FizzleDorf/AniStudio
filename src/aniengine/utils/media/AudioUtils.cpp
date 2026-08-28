// AudioUtils.cpp
#include "AudioUtils.hpp"
#include "MetadataUtils.hpp"
#include "VideoMetadataUtils.hpp"

namespace Utils {

    bool AudioUtils::HasExifMetadata(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return false;
            meta = MetadataUtils::NormalizeAniStudioMetadata(meta);

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
            return false;
        }
        catch (...) {
            return false;
        }
    }

    bool AudioUtils::HasLSBMetadata(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return false;
            meta = MetadataUtils::NormalizeAniStudioMetadata(meta);

            std::vector<std::string> stealthKeys = { "LSB", "Stealth", "Hidden", "steganography", "lsb" };
            for (const auto& key : stealthKeys) {
                if (meta.contains(key)) return true;
            }

            if (meta.contains("components") && meta["components"].is_array()) {
                for (const auto& comp : meta["components"]) {
                    if (comp.is_object()) {
                        for (const auto& key : stealthKeys) {
                            if (comp.contains(key)) return true;
                        }
                    }
                }
            }
            return false;
        }
        catch (...) {
            return false;
        }
    }

    int AudioUtils::GetMetadataStatus(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return 0;
            meta = MetadataUtils::NormalizeAniStudioMetadata(meta);

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
            return 0;
        }
        catch (...) {
            return 0;
        }
    }

} // namespace Utils