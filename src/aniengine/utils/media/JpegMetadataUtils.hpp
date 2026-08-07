#pragma once

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

namespace Utils {

    class JpegMetadata {
    public:
        static bool WriteMetadataToJPEG(const std::string& imagePath, const nlohmann::json& metadata) {
#ifdef USE_EXIV2
            try {
                Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(imagePath);
                if (!image.get()) {
                    std::cerr << "Failed to open JPEG with Exiv2: " << imagePath << std::endl;
                    return false;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                std::string jsonStr = metadata.dump();

                Exiv2::Value::AutoPtr value = Exiv2::Value::create(Exiv2::asciiString);
                value->read(jsonStr);
                exifData["Exif.Photo.UserComment"] = value;

                image->writeMetadata();
                std::cout << "Successfully wrote EXIF metadata to JPEG: " << imagePath << std::endl;
                return true;
            }
            catch (const Exiv2::Error& e) {
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
                return false;
            }
#else
            // Fallback: write sidecar JSON
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::SaveMetadataToJson(jsonPath, metadata);
#endif
        }

        static nlohmann::json ReadMetadataFromJPEG(const std::string& imagePath) {
#ifdef USE_EXIV2
            nlohmann::json result;
            try {
                Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(imagePath);
                if (!image.get()) {
                    std::cerr << "Failed to open JPEG with Exiv2: " << imagePath << std::endl;
                    return result;
                }

                image->readMetadata();
                Exiv2::ExifData& exifData = image->exifData();
                Exiv2::ExifData::const_iterator it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.UserComment"));
                if (it != exifData.end()) {
                    try {
                        result = nlohmann::json::parse(it->toString());
                        std::cout << "Successfully loaded EXIF metadata from JPEG" << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error parsing JSON from EXIF: " << e.what() << std::endl;
                    }
                }
            }
            catch (const Exiv2::Error& e) {
                std::cerr << "Exiv2 error: " << e.what() << std::endl;
            }
            return result;
#else
            // Fallback: read sidecar JSON
            std::string jsonPath = imagePath + ".json";
            return MetadataUtils::LoadMetadataFromJson(jsonPath);
#endif
        }
    };

} // namespace Utils