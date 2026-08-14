#include "SteganographyUtils.hpp"
#include <iostream>
#include <cstring>

namespace Utils {

    void SteganographyUtils::writeBit(unsigned char* pixelAlpha, int bit) {
        if (bit)
            *pixelAlpha |= 1;
        else
            *pixelAlpha &= ~1;
    }

    int SteganographyUtils::readBit(const unsigned char* pixelAlpha) {
        return (*pixelAlpha) & 1;
    }

    bool SteganographyUtils::EmbedInAlpha(unsigned char* rgbaData, int width, int height,
        const std::vector<unsigned char>& payload,
        const std::string& signature) {
        if (!rgbaData || width <= 0 || height <= 0 || payload.empty())
            return false;

        // Build full payload: signature + 32-bit length (bits) + data bits
        std::vector<unsigned char> fullPayload;
        fullPayload.reserve(signature.size() * 8 + 32 + payload.size() * 8);

        // Signature bits (8 bits per char)
        for (char c : signature) {
            for (int bit = 7; bit >= 0; --bit) {
                fullPayload.push_back((c >> bit) & 1);
            }
        }

        // Length (number of bits of the original payload) as 32-bit big-endian
        uint32_t payloadLenBits = static_cast<uint32_t>(payload.size() * 8);
        for (int bit = 31; bit >= 0; --bit) {
            fullPayload.push_back((payloadLenBits >> bit) & 1);
        }

        // Data bits (each byte as 8 bits)
        for (unsigned char byte : payload) {
            for (int bit = 7; bit >= 0; --bit) {
                fullPayload.push_back((byte >> bit) & 1);
            }
        }

        size_t totalBits = fullPayload.size();
        size_t maxBits = width * height; // one bit per pixel (alpha LSB)

        if (totalBits > maxBits) {
            std::cerr << "Payload too large to embed in alpha channel" << std::endl;
            return false;
        }

        const int row_stride = width * 4;
        size_t bitOffset = 0;

        // Column-major order (x outer, y inner)
        for (int x = 0; x < width && bitOffset < totalBits; ++x) {
            for (int y = 0; y < height && bitOffset < totalBits; ++y) {
                unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
                writeBit(pixelAlpha, fullPayload[bitOffset]);
                bitOffset++;
            }
        }

        return true;
    }

    std::vector<unsigned char> SteganographyUtils::ExtractFromAlpha(const unsigned char* rgbaData,
        int width, int height,
        const std::string& signature) {
        std::vector<unsigned char> result;
        if (!rgbaData || width <= 0 || height <= 0)
            return result;

        const int row_stride = width * 4;
        size_t bitOffset = 0;

        // First, read signature bits
        std::string extractedSig;
        for (char c : signature) {
            unsigned char byte = 0;
            for (int bit = 7; bit >= 0; --bit) {
                if (bitOffset >= width * height) {
                    std::cerr << "Not enough pixels for signature" << std::endl;
                    return result;
                }
                int x = bitOffset % width;
                int y = bitOffset / width;
                const unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
                byte = (byte << 1) | readBit(pixelAlpha);
                bitOffset++;
            }
            extractedSig.push_back(static_cast<char>(byte));
            if (extractedSig.back() != c) {
                std::cerr << "Signature mismatch" << std::endl;
                return result;
            }
        }

        // Read length (32 bits)
        uint32_t payloadLenBits = 0;
        for (int i = 0; i < 32; ++i) {
            if (bitOffset >= width * height) {
                std::cerr << "Not enough pixels for length" << std::endl;
                return result;
            }
            int x = bitOffset % width;
            int y = bitOffset / width;
            const unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
            payloadLenBits = (payloadLenBits << 1) | readBit(pixelAlpha);
            bitOffset++;
        }

        size_t numBytes = (payloadLenBits + 7) / 8;
        result.reserve(numBytes);

        for (size_t byteIdx = 0; byteIdx < numBytes; ++byteIdx) {
            unsigned char byte = 0;
            int bitsToRead = (byteIdx == numBytes - 1) ? (payloadLenBits % 8) : 8;
            if (bitsToRead == 0) bitsToRead = 8;
            for (int b = 0; b < bitsToRead; ++b) {
                if (bitOffset >= width * height) {
                    std::cerr << "Not enough pixels for payload" << std::endl;
                    return result;
                }
                int x = bitOffset % width;
                int y = bitOffset / width;
                const unsigned char* pixelAlpha = rgbaData + y * row_stride + x * 4 + 3;
                byte = (byte << 1) | readBit(pixelAlpha);
                bitOffset++;
            }
            if (bitsToRead < 8) byte <<= (8 - bitsToRead);
            result.push_back(byte);
        }

        return result;
    }

} // namespace Utils