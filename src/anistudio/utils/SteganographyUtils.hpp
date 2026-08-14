#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace Utils {

    class SteganographyUtils {
    public:
        // Embed a binary payload into the LSB of the alpha channel of RGBA data.
        // Returns true if successful (payload fits).
        static bool EmbedInAlpha(unsigned char* rgbaData, int width, int height,
            const std::vector<unsigned char>& payload,
            const std::string& signature = "stealth_");

        // Extract a binary payload from the LSB of the alpha channel.
        static std::vector<unsigned char> ExtractFromAlpha(const unsigned char* rgbaData,
            int width, int height,
            const std::string& signature = "stealth_");

    private:
        static void writeBit(unsigned char* pixelAlpha, int bit);
        static int readBit(const unsigned char* pixelAlpha);
    };

} // namespace Utils