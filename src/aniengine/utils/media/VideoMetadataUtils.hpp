#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Utils {

    class VideoMetadataUtils {
    public:
        static bool WriteMetadataToVideo(const std::string& videoPath,
            const nlohmann::json& metadata,
            bool forceSidecar = false);

        static nlohmann::json ReadMetadataFromVideo(const std::string& videoPath);
    };

} // namespace Utils