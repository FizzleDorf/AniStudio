#include "MetadataUtils.hpp"
#include "VideoMetadataUtils.hpp"

#include <vector>

namespace Utils {

    // These were moved here from the deleted AudioUtils class. They are the
    // only functions in MetadataUtils that need a translation unit, since
    // VideoMetadataUtils::ReadMetadataFromVideo is not header-only.

    bool MetadataUtils::HasExifMetadata(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return false;
            meta = NormalizeAniStudioMetadata(meta);

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

    bool MetadataUtils::HasLSBMetadata(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return false;
            meta = NormalizeAniStudioMetadata(meta);

            static const std::vector<std::string> stealthKeys = {
                "LSB", "Stealth", "Hidden", "steganography", "lsb"
            };
            for (const auto& k : stealthKeys) {
                if (meta.contains(k)) return true;
            }
            if (meta.contains("components") && meta["components"].is_array()) {
                for (const auto& comp : meta["components"]) {
                    if (comp.is_object()) {
                        for (const auto& k : stealthKeys) {
                            if (comp.contains(k)) return true;
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

    int MetadataUtils::GetMetadataStatus(const std::string& filePath) {
        try {
            nlohmann::json meta = VideoMetadataUtils::ReadMetadataFromVideo(filePath);
            if (meta.is_null() || meta.empty()) return 0;
            meta = NormalizeAniStudioMetadata(meta);

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
            if (meta.contains("dataType") && meta["dataType"] == "entity" &&
                meta.contains("data")) {
                return 1;
            }
            return 0;
        }
        catch (...) {
            return 0;
        }
    }

} // namespace Utils