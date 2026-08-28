// AudioUtils.hpp
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Utils {

    class AudioUtils {
    public:
        static bool HasExifMetadata(const std::string& filePath);
        static bool HasLSBMetadata(const std::string& filePath);
        static int GetMetadataStatus(const std::string& filePath);
    };

} // namespace Utils