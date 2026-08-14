#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Utils {

    class WebPMetadata {
    public:
        static bool WriteMetadataToWebP(const std::string& imagePath, const nlohmann::json& metadata);
        static nlohmann::json ReadMetadataFromWebP(const std::string& imagePath);
    };

} // namespace Utils