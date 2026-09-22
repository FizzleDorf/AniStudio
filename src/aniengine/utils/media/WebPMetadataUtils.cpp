#include "WebPMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include "Log.hpp"

#include <memory>

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

namespace Utils {

    bool WebPMetadata::WriteMetadataToWebP(const std::string& imagePath, const nlohmann::json& metadata) {
#ifdef USE_EXIV2
        try {
            auto image = Exiv2::ImageFactory::open(imagePath);
            if (!image) {
                ANI_LOG_WARN("Failed to open WebP with Exiv2: %s", imagePath.c_str());
                return false;
            }
            image->readMetadata();
            Exiv2::XmpData& xmpData = image->xmpData();
            std::string jsonStr = metadata.dump();
            auto value = Exiv2::Value::create(Exiv2::asciiString);
            value->read(jsonStr);
            xmpData["Xmp.aniStudio.params"] = *value;
            image->writeMetadata();
            ANI_LOG_DEBUG("Wrote WebP metadata: %s", imagePath.c_str());
            return true;
        }
        catch (const Exiv2::Error& e) {
            ANI_LOG_ERROR("Exiv2 error writing WebP %s: %s", imagePath.c_str(), e.what());
            return false;
        }
#else
        std::string jsonPath = imagePath + ".json";
        return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
#endif
    }

    nlohmann::json WebPMetadata::ReadMetadataFromWebP(const std::string& imagePath) {
#ifdef USE_EXIV2
        nlohmann::json result;
        try {
            auto image = Exiv2::ImageFactory::open(imagePath);
            if (!image) {
                ANI_LOG_WARN("Failed to open WebP with Exiv2: %s", imagePath.c_str());
                return result;
            }

            image->readMetadata();

            Exiv2::XmpData& xmpData = image->xmpData();
            for (const auto& xmp : xmpData) {
                std::string key = xmp.key();
                std::string value = xmp.toString();
                try {
                    nlohmann::json parsed = nlohmann::json::parse(value);
                    if (parsed.is_object() || parsed.is_array()) {
                        result[key] = parsed;
                    }
                    else {
                        result[key] = value;
                    }
                }
                catch (...) {
                    result[key] = value;
                }
            }

            Exiv2::ExifData& exifData = image->exifData();
            for (const auto& exif : exifData) {
                std::string key = exif.key();
                std::string value = exif.toString();
                try {
                    nlohmann::json parsed = nlohmann::json::parse(value);
                    if (parsed.is_object() || parsed.is_array()) {
                        result[key] = parsed;
                    }
                    else {
                        result[key] = value;
                    }
                }
                catch (...) {
                    result[key] = value;
                }
            }
        }
        catch (const Exiv2::Error& e) {
            ANI_LOG_ERROR("Exiv2 error reading WebP %s: %s", imagePath.c_str(), e.what());
        }
        return result;
#else
        std::string jsonPath = imagePath + ".json";
        return MetadataUtils::LoadMetadataFromJson(jsonPath);
#endif
    }

} // namespace Utils