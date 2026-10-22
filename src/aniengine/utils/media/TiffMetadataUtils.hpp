#pragma once

#include "Log.hpp"

#include <string>
#include <nlohmann/json.hpp>

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

namespace Utils {

    class TIFFMetadata {
    public:
        static bool WriteMetadataToTIFF(const std::string& imagePath, const nlohmann::json& metadata) {
#ifdef USE_EXIV2
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    ANI_LOG_WARN("Failed to open TIFF with Exiv2: %s", imagePath.c_str());
                    return false;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                std::string jsonStr = metadata.dump();

                auto value = Exiv2::Value::create(Exiv2::asciiString);
                value->read(jsonStr);
                exifData["Exif.Image.ImageDescription"] = *value;

                image->writeMetadata();
                ANI_LOG_DEBUG("Wrote EXIF metadata to TIFF: %s", imagePath.c_str());
                return true;
            }
            catch (const Exiv2::Error& e) {
                ANI_LOG_ERROR("Exiv2 error writing TIFF %s: %s", imagePath.c_str(), e.what());
                return false;
            }
#else
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
#endif
        }

        static nlohmann::json ReadMetadataFromTIFF(const std::string& imagePath) {
#ifdef USE_EXIV2
            nlohmann::json result;
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    ANI_LOG_WARN("Failed to open TIFF with Exiv2: %s", imagePath.c_str());
                    return result;
                }

                image->readMetadata();

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
            }
            catch (const Exiv2::Error& e) {
                ANI_LOG_ERROR("Exiv2 error reading TIFF %s: %s", imagePath.c_str(), e.what());
            }
            return result;
#else
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::LoadMetadataFromJson(jsonPath);
#endif
        }
    };

} // namespace Utils