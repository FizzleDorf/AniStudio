#include "WebPMetadataUtils.hpp"
#include "MetadataUtils.hpp"
#include <iostream>
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
                std::cerr << "Failed to open WebP with Exiv2: " << imagePath << std::endl;
                return false;
            }
            image->readMetadata();
            Exiv2::XmpData& xmpData = image->xmpData();
            std::string jsonStr = metadata.dump();
            auto value = Exiv2::Value::create(Exiv2::asciiString);
            value->read(jsonStr);
            xmpData["Xmp.aniStudio.params"] = *value;
            image->writeMetadata();
            std::cout << "WebP metadata written: " << imagePath << std::endl;
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

    nlohmann::json WebPMetadata::ReadMetadataFromWebP(const std::string& imagePath) {
#ifdef USE_EXIV2
        nlohmann::json result;
        try {
            auto image = Exiv2::ImageFactory::open(imagePath);
            if (!image) {
                std::cerr << "Failed to open WebP with Exiv2: " << imagePath << std::endl;
                return result;
            }
            image->readMetadata();
            Exiv2::XmpData& xmpData = image->xmpData();
            auto it = xmpData.findKey(Exiv2::XmpKey("Xmp.aniStudio.params"));
            if (it != xmpData.end()) {
                try {
                    result = nlohmann::json::parse(it->toString());
                    std::cout << "WebP metadata loaded from XMP" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "Error parsing JSON from XMP: " << e.what() << std::endl;
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

} // namespace Utils