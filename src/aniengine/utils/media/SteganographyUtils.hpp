#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Utils {

    class SteganographyUtils {
    public:
        static bool EmbedInAlpha(unsigned char* rgbaData, int width, int height,
            const std::vector<unsigned char>& payload,
            const std::string& signature = "");

        static std::vector<unsigned char> ExtractFromAlpha(const unsigned char* rgbaData,
            int width, int height,
            const std::string& signature = "");

    private:
        static void writeBit(unsigned char* pixelAlpha, int bit);
        static int readBit(const unsigned char* pixelAlpha);
    };

}