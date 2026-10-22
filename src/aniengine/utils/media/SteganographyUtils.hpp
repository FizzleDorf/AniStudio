#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Utils {

    class SteganographyUtils {
    public:
        static bool EmbedInAlpha(unsigned char* rgbaData, int width, int height,
            const std::vector<unsigned char>& payload,
            const std::string& signature = "");

        static std::vector<unsigned char> ExtractFromAlpha(const unsigned char* rgbaData,
            int width, int height,
            const std::string& signature = "");

        static nlohmann::json ExtractJsonFromAlpha(const unsigned char* rgbaData,
            int width, int height);

        static nlohmann::json ExtractJsonFromAlpha(const unsigned char* rgbaData,
            int width, int height,
            std::vector<unsigned char>* outRaw);

        static bool LooksLikeAniStudioMetadata(const nlohmann::json& j);

    private:
        static void writeBit(unsigned char* pixelAlpha, int bit);
        static int readBit(const unsigned char* pixelAlpha);

        struct BitReader {
            const unsigned char* data;
            int width;
            int height;
            size_t bitIndex = 0;

            bool ok() const {
                return data != nullptr && width > 0 && height > 0 &&
                    bitIndex < static_cast<size_t>(width) * height;
            }
            int readBit();
            unsigned char readByteMSB();
            uint32_t readU32BE();
            bool readBytes(std::vector<unsigned char>& out, size_t count);
        };
    };

}