#pragma once

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
#include <memory>

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

namespace Utils {

    class JpegMetadata {
    public:
        static bool WriteMetadataToJPEG(const std::string& imagePath, const nlohmann::json& metadata) {
#ifdef USE_EXIV2
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    std::cerr << "Failed to open JPEG with Exiv2: " << imagePath << std::endl;
                    return false;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                std::string jsonStr = metadata.dump();

                auto value = Exiv2::Value::create(Exiv2::asciiString);
                value->read(jsonStr);
                exifData["Exif.Photo.UserComment"] = *value;

                image->writeMetadata();
                std::cout << "Successfully wrote EXIF metadata to JPEG: " << imagePath << std::endl;
                return true;
            }
            catch (const Exiv2::Error& e) {
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
                return false;
            }
#else
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
#endif
        }

        static nlohmann::json ReadMetadataFromJPEG(const std::string& imagePath) {
#ifdef USE_EXIV2
            nlohmann::json result;
            try {
                auto image = Exiv2::ImageFactory::open(imagePath);
                if (!image) {
                    std::cerr << "Failed to open JPEG with Exiv2: " << imagePath << std::endl;
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
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
            }
            return result;
#else
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::LoadMetadataFromJson(jsonPath);
#endif
        }
    };

}